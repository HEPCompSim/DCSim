/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <wrench.h>
#include "SimpleSimulator.h"
#include "SimpleExecutionController.h"
#include "JobSpecification.h"

#include <iostream>
#include <fstream>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

/**
 *
 * "Global" static variables. Some here are a bit ugly of course, but they should help
 * with memory footprint by avoiding passing around / storing items that apply to
 * all jobs.
 */
std::map<std::shared_ptr<wrench::StorageService>, LRU_FileList> SimpleSimulator::global_file_map;
std::mt19937 SimpleSimulator::gen(42);  // random number generator
bool SimpleSimulator::use_blockstreaming = true;   // flag to chose between simulated job types: streaming or copy jobs
double SimpleSimulator::xrd_block_size = 1.*1000*1000*1000; // maximum size of the streamed file blocks in bytes for the XRootD-ish streaming
// TODO: The initialized below is likely bogus (at compile time?)
std::normal_distribution<double>* SimpleSimulator::flops_dist;
std::normal_distribution<double>* SimpleSimulator::mem_dist;
std::normal_distribution<double>* SimpleSimulator::insize_dist;
std::normal_distribution<double>* SimpleSimulator::outsize_dist;
std::set<std::string> SimpleSimulator::cache_hosts;
std::set<std::string> SimpleSimulator::storage_hosts;
std::set<std::string> SimpleSimulator::worker_hosts;
std::set<std::string> SimpleSimulator::scheduler_hosts;
std::set<std::string> SimpleSimulator::executors;
std::set<std::string> SimpleSimulator::file_registries;
std::set<std::string> SimpleSimulator::network_monitors;




/**
 * @brief helper function to process simulation options and parameters
 * 
 * @param argc
 * @param argv 
 * 
 */
po::variables_map process_program_options(int argc, char** argv) {

    // default values
    double hitrate = 0.0;

    double average_flops = 2164.428*1000*1000*1000;
    double sigma_flops = 0.1*average_flops;
    double average_memory = 2.*1000*1000*1000;
    double sigma_memory = 0.1*average_memory;
    size_t infiles_per_job = 10;
    double average_infile_size = 3600000000.;
    double sigma_infile_size = 0.1*average_infile_size;
    double average_outfile_size = 0.5*infiles_per_job*average_infile_size;
    double sigma_outfile_size = 0.1*average_outfile_size;

    size_t duplications = 1;

    bool no_blockstreaming = false;

    double xrd_block_size = 1000.*1000*1000;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "show brief usage message\n")

        ("platform,p", po::value<std::string>()->value_name("<platform>")->required(), "platform description file, written in XML following the SimGrid-defined DTD")
        ("hitrate,H", po::value<double>()->default_value(hitrate), "initial fraction of staged input-files on caches at simulation start")

        ("njobs,n", po::value<size_t>()->required(), "number of jobs to simulate")
        ("flops", po::value<double>()->default_value(average_flops), "amount of floating point operations jobs need to process")
        ("sigma-flops", po::value<double>()->default_value(sigma_flops), "jobs' distribution spread in FLOPS")
        ("mem,m", po::value<double>()->default_value(average_memory), "average size of memory needed for jobs to run")
        ("sigma-mem", po::value<double>()->default_value(sigma_memory), "jobs' sistribution spread in memory-needs")
        ("ninfiles", po::value<size_t>()->default_value(infiles_per_job), "number of input-files each job has to process")
        ("insize", po::value<double>()->default_value(average_infile_size), "average size of input-files jobs read")
        ("sigma-insize", po::value<double>()->default_value(sigma_infile_size), "jobs' distribution spread in input-file size")
        ("outsize", po::value<double>()->default_value(average_outfile_size), "average size of output-files jobs write")
        ("sigma-outsize", po::value<double>()->default_value(sigma_outfile_size), "jobs' distribution spread in output-file size")

        ("duplications,d", po::value<size_t>()->default_value(duplications), "number of duplications of the workflow to feed into the simulation")

        ("no-streaming", po::bool_switch()->default_value(no_blockstreaming), "switch to turn on/off block-wise streaming of input-files")

        ("output-file,o", po::value<std::string>()->value_name("<out file>")->required(), "path for the CSV file containing output information about the jobs in the simulation")

        ("xrd-blocksize,x", po::value<double>()->default_value(xrd_block_size), "size of the blocks XRootD uses for data streaming")
    ;

    po::variables_map vm;
    po::store(
        po::parse_command_line(argc, argv, desc),
        vm
    );

    if (vm.count("help")) {
        std::cerr << desc << std::endl;
        exit(EXIT_SUCCESS);
    }

    try {
        po::notify(vm);
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl << std::endl;
        std::cerr << desc << std::endl;
        exit(EXIT_FAILURE);
    }

    // Here, all options should be properly set
    std::cerr << "Using platform " << vm["platform"].as<std::string>() << std::endl;

    return vm;
}


/**
 * @brief Fill a Workflow consisting of jobs with job specifications, 
 * which include the inputfile and outputfile dependencies.
 * It can be chosen between jobs streaming input data and perform computations simultaneously 
 * or jobs copying the full input-data and compute afterwards.
 *    
 * @param num_jobs: number of tasks
 * @param infiles_per_task: number of input-files each job processes
 * @param average_flops: expectation value of the flops (truncated gaussian) distribution
 * @param sigma_flops: std. deviation of the flops (truncated gaussian) distribution
 * @param average_memory: expectation value of the memory (truncated gaussian) distribution
 * @param sigma_memory: std. deviation of the memory (truncated gaussian) distribution
 * @param average_infile_size: expectation value of the input-file size (truncated gaussian) distribution
 * @param sigma_infile_size: std. deviation of the input-file size (truncated gaussian) distribution
 * @param average_outfile_size: expectation value of the output-file size (truncated gaussian) distribution
 * @param sigma_outfile_size: std. deviation of the output-file size (truncated gaussian) distribution
 * @param duplications: number of duplications of the workflow to feed into the simulation
 * 
 * @throw std::runtime_error
 */
std::map<std::string, JobSpecification> fill_streaming_workflow (
        size_t num_jobs,
        size_t infiles_per_task,
        double average_flops, double sigma_flops,
        double average_memory, double sigma_memory,
        double average_infile_size, double sigma_infile_size,
        double average_outfile_size, double sigma_outfile_size,
        size_t duplications
) {

    // Map to store the workload specification
    std::map<std::string, JobSpecification> workload;

    // Initialize random number generators
    std::normal_distribution<> flops_dist(average_flops, sigma_flops);
    std::normal_distribution<> mem_dist(average_memory, sigma_memory);
    std::normal_distribution<> insize_dist(average_infile_size, sigma_infile_size);
    std::normal_distribution<> outsize_dist(average_outfile_size,sigma_outfile_size);

    for (size_t j = 0; j < num_jobs; j++) {

        // Create a job specification
        JobSpecification job_specification;

        // Sample strictly positive task flops
        double dflops = flops_dist(SimpleSimulator::gen);
        while ((average_flops+sigma_flops) < dflops || dflops < 0.) dflops = flops_dist(SimpleSimulator::gen);
        job_specification.total_flops = dflops;

        // Sample strictly positive task memory requirements
        double dmem = mem_dist(SimpleSimulator::gen);
        while ((average_memory+sigma_memory) < dmem || dmem < 0.) dmem = mem_dist(SimpleSimulator::gen);
        job_specification.total_mem = dmem;

        for (size_t f = 0; f < infiles_per_task; f++) {
            // Sample inputfile sizes
            double dinsize = insize_dist(SimpleSimulator::gen);
            while ((average_infile_size+3*sigma_infile_size) < dinsize || dinsize < 0.) dinsize = insize_dist(SimpleSimulator::gen);
            job_specification.infiles.push_back(wrench::Simulation::addFile("infile_" + std::to_string(j) + "_" + std::to_string(f), dinsize));
        }

        // Sample outfile sizes
        double doutsize = outsize_dist(SimpleSimulator::gen);
        while ((average_outfile_size+3*sigma_outfile_size) < doutsize || doutsize < 0.) doutsize = outsize_dist(SimpleSimulator::gen);
        job_specification.outfile = wrench::Simulation::addFile("outfile_" + std::to_string(j), doutsize);

        for (size_t d=0; d < duplications; d++) {
            workload["job_" + std::to_string(j+d)] = job_specification;
        }
    }
    return workload;
}

/**
 * @brief Identify demanded services on hosts to run based on configured "type" property tag
 * 
 * @param simulation Simulation object with already instantiated hosts
 * 
 * @throw std::runtime_error, std::invalid_argument
 */
void SimpleSimulator::identifyHostTypes(std::shared_ptr<wrench::Simulation> simulation){
    std::vector<std::string> hostname_list = simulation->getHostnameList();
    if (hostname_list.size() == 0) {
        throw std::runtime_error("Empty hostname list! Have you instantiated the platform already?");
    }
    for (const auto& hostname: hostname_list) {
        auto hostProperties = wrench::S4U_Simulation::getHostProperty(hostname, "type");
        bool validType = false;
        if (hostProperties == ""){
            throw std::runtime_error("Configuration property \"type\" missing for host " + hostname);
        }
        if (hostProperties.find("executor") != std::string::npos) {
            SimpleSimulator::executors.insert(hostname);
        }
        if (hostProperties.find("fileregistry") != std::string::npos) {
            SimpleSimulator::file_registries.insert(hostname);
        }
        if (hostProperties.find("networkmonitor") != std::string::npos) {
            SimpleSimulator::network_monitors.insert(hostname);
        }
        if (hostProperties.find("storage") != std::string::npos) {
            SimpleSimulator::storage_hosts.insert(hostname);
        }
        if (hostProperties.find("cache") != std::string::npos) {
            SimpleSimulator::cache_hosts.insert(hostname);
        }
        if (hostProperties.find("worker") != std::string::npos) {
            SimpleSimulator::worker_hosts.insert(hostname);
        }
        if (hostProperties.find("scheduler") != std::string::npos) {
            SimpleSimulator::scheduler_hosts.insert(hostname);
        }
        if (! validType) {
            throw std::runtime_error("Invalid type " + hostProperties + " configuration in host " + hostname + "!");
        }
    }
}


int main(int argc, char **argv) {

    // instantiate a simulation
    auto simulation = wrench::Simulation::createSimulation();

    // Initialization of the simulation
    simulation->init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    auto vm = process_program_options(argc, argv);

    // The first argument is the platform description file, written in XML following the SimGrid-defined DTD
    std::string platform_file = vm["platform"].as<std::string>();

    // output-file name containing simulation information
    std::string filename = vm["output-file"].as<std::string>();

    size_t num_jobs = vm["njobs"].as<size_t>();
    size_t infiles_per_job = vm["ninfiles"].as<size_t>();
    double hitrate = vm["hitrate"].as<double>();

    double average_flops = vm["flops"].as<double>();
    double sigma_flops = vm["sigma-flops"].as<double>();
    double average_memory = vm["mem"].as<double>();
    double sigma_memory = vm["sigma-mem"].as<double>();
    double average_infile_size = vm["insize"].as<double>();
    double sigma_infile_size = vm["sigma-insize"].as<double>();
    double average_outfile_size = vm["outsize"].as<double>();
    double sigma_outfile_size = vm["sigma-outsize"].as<double>();

    size_t duplications = vm["duplications"].as<size_t>();

    // Flags to turn on/off blockwise streaming of input-files
    SimpleSimulator::use_blockstreaming = !(vm["no-streaming"].as<bool>());

    // Set XRootD block size
    SimpleSimulator::xrd_block_size = vm["xrd-blocksize"].as<double>();


    /* Create a workload */
    std::cerr << "Constructing workload specification..." << std::endl;

    auto workload_spec = fill_streaming_workflow(
        num_jobs, infiles_per_job,
        average_flops, sigma_flops,
        average_memory,sigma_memory,
        average_infile_size, sigma_infile_size,
        average_outfile_size, sigma_outfile_size,
        duplications
    );

    std::cerr << "The workflow has " << std::to_string(num_jobs) << " jobs" << std::endl;


    /* Read and parse the platform description file to instantiate a simulation platform */
    std::cerr << "Instantiating SimGrid platform..." << std::endl;
    simulation->instantiatePlatform(platform_file);


    /* Identify demanded and create storage and compute services and add them to the simulation */
    SimpleSimulator::identifyHostTypes(simulation);

    // Create a list of cache storage services
    std::set<std::shared_ptr<wrench::StorageService>> cache_storage_services;
    for (auto host: SimpleSimulator::cache_hosts) {
        //TODO: Support more than one type of cache mounted differently?
        //TODO: This might not be necessary since different cache layers are typically on different hosts
        auto storage_service = simulation->add(new wrench::SimpleStorageService(host, {"/"}));
        cache_storage_services.insert(storage_service);
    }

    // and remote storages that are able to serve all file requests
    //TODO: Think of a way to support grid storages serving only some datasets
    std::set<std::shared_ptr<wrench::StorageService>> grid_storage_services;
    for (auto host: SimpleSimulator::storage_hosts) {
        auto storage_service = simulation->add(new wrench::SimpleStorageService(host, {"/"}));
        grid_storage_services.insert(storage_service);
    }

    // Create a list of compute services that will be used by the HTCondorService
    std::set<std::shared_ptr<wrench::ComputeService>> condor_compute_resources;
    for (auto host: SimpleSimulator::worker_hosts) {
        condor_compute_resources.insert(
            simulation->add(
                new wrench::BareMetalComputeService(
                    host,
                    {std::make_pair(
                        host,
                        std::make_tuple(
                            wrench::Simulation::getHostNumCores(host),
                            wrench::Simulation::getHostMemoryCapacity(host)
                        )
                    )},
                    ""
                )
            )
        );
    }

    // Instantiate a HTcondorComputeService and add it to the simulation
    std::set<std::shared_ptr<wrench::HTCondorComputeService>> htcondor_compute_services;
    //TODO: Think of a way to support more than one HTCondor scheduler
    if (SimpleSimulator::scheduler_hosts.size() != 1) {
        throw std::runtime_error("Currently this simulator supports only a single HTCondor scheduler!");
    }
    for (auto host: SimpleSimulator::scheduler_hosts) {
        htcondor_compute_services.insert(
            simulation->add(
                new wrench::HTCondorComputeService(
                    host,
                    condor_compute_resources,
                    {
                        {wrench::HTCondorComputeServiceProperty::NEGOTIATOR_OVERHEAD, "1.0"},
                        {wrench::HTCondorComputeServiceProperty::GRID_PRE_EXECUTION_DELAY, "10.0"},
                        {wrench::HTCondorComputeServiceProperty::GRID_POST_EXECUTION_DELAY, "10.0"},
                        {wrench::HTCondorComputeServiceProperty::NON_GRID_PRE_EXECUTION_DELAY, "5.0"},
                        {wrench::HTCondorComputeServiceProperty::NON_GRID_POST_EXECUTION_DELAY, "5.0"}
                    },
                    {}
                )
            )
        );
    }


    /* Instantiate file registry services */
    std::set<std::shared_ptr<wrench::FileRegistryService>> file_registry_services;
    for (auto host: SimpleSimulator::file_registries) {
        std::cerr << "Instantiating a FileRegistryService on " << host << "..." << std::endl;
        auto file_registry_service = simulation->add(new wrench::FileRegistryService(host));
        file_registry_services.insert(file_registry_service);
    }


    /* Instantiate Execution Controllers */
    std::set<std::shared_ptr<SimpleExecutionController>> execution_controllers;
    //TODO: Think of a way to support more than one execution controller
    if (SimpleSimulator::executors.size() != 1) {
        throw std::runtime_error("Currently this simulator supports only a single execution controller!");
    }
    for (auto host: SimpleSimulator::executors) {
        auto wms = simulation->add(
            new SimpleExecutionController(
                workload_spec,
                htcondor_compute_services,
                grid_storage_services,
                cache_storage_services,
                host,
                //hitrate,
                filename
            )
        );
        execution_controllers.insert(wms);
    }
    

    /* Instantiate inputfiles and set outfile destinations*/
    auto wms = *execution_controllers.begin();
    std::cerr << "Creating and staging input files plus set destination of output files..." << std::endl;
    try {
        for (auto &job_name_spec: wms->get_workload_spec()) {
            // job specifications
            auto &job_spec = job_name_spec.second;
            std::shuffle(job_spec.infiles.begin(), job_spec.infiles.end(), SimpleSimulator::gen); // Shuffle the input files
            // Compute the job's incremental inputfiles size
            double incr_inputfile_size = 0.;
            for (auto const &f : job_spec.infiles) {
                incr_inputfile_size += f->getSize();
            }
            double cached_files_size = 0.;
            for (auto const &f : job_spec.infiles) {
                // Distribute the inputfiles on all GRID storages
                //TODO: Think of a more realistic distribution pattern and avoid duplications
                for (auto storage_service: grid_storage_services) {
                    simulation->stageFile(f, storage_service);
                    SimpleSimulator::global_file_map[storage_service].touchFile(f);
                }
                // Distribute the infiles on all caches until desired hitrate is reached
                if (cached_files_size < hitrate*incr_inputfile_size) {
                    for (const auto& cache : cache_storage_services) {
                        simulation->stageFile(f, cache);
                        SimpleSimulator::global_file_map[cache].touchFile(f);
                    }
                    cached_files_size += f->getSize();
                }
            }
            if (cached_files_size/incr_inputfile_size < hitrate) {
                throw std::runtime_error("Desired hitrate was not reached!");
            }

            // Set outfile destinations
            // TODO: Think of a way to identify a specific (GRID) storage
            for (auto storage_service: grid_storage_services) {
                job_spec.outfile_destination = wrench::FileLocation::LOCATION(storage_service);
                break;
            }
        }
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 0;
    }


    /* Launch the simulation */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation->launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 0;
    }
    std::cerr << "Simulation done!" << std::endl;


    return 0;
}
