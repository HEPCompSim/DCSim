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
#include "SimpleWMS.h"

#include <iostream>
#include <fstream>

#include <boost/program_options.hpp>

namespace po = boost::program_options;


std::mt19937 gen(42);


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

    bool use_blockstreaming = true;
    bool use_simplified_blockstreaming = false;

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

        ("blockstreaming", po::bool_switch()->default_value(true), "switch to turn on/off block-wise streaming of input-files")
        ("simplified-blockstreaming", po::bool_switch()->default_value(false), "switch to turn on/off simplified input-file streaming")

        ("output-file,o", po::value<std::string>()->value_name("<out file>")->required(), "path for the CSV file containing output information about the jobs in the simulation")
    ;

    po::variables_map vm;
    po::store(
        po::parse_command_line(argc, argv, desc),
        vm
    );

    if (vm.count("help")) {
        std::cerr << desc << std::endl;
        return 0;
    }
    if (vm.count("platform")) {
        std::cerr << "Using platform " << vm["platform"].as<std::string>() << std::endl;
    }
    else {
        std::cerr << "Platform must be but is not set!" << std::endl;
    }

    try {
        po::notify(vm);
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << desc << std::endl;
        exit(EXIT_FAILURE);
    }
    return vm;
}


/**
 * @brief fill a Workflow with tasks, which include the inputfile and outputfile dependencies of a job.
 * Optionally a task chain which takes care of streaming input data and perform computations in blocks 
 * per job can be created in a simplified and fully XRootD-ish manner.
 *    
 * @param workflow: Workflow to fill with tasks
 * @param use_blockstreaming: switch to turn on blockwise streaming, else wait for inputfile copy
 * @param use_simplified_blockstreaming: switch to turn on simplified blockwise streaming (1 block) of input data, when blockwise streaming true
 * @param xrd_block_size: maximum size of the streamed file blocks in bytes for the XRootD-ish streaming
 * @param dummy_flops: numer of flops each dummy task is executing
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
 * 
 * @throw std::runtime_error
 */
void fill_streaming_workflow (
    wrench::Workflow* workflow,
    size_t num_jobs,
    size_t infiles_per_task,
    double average_flops, double sigma_flops,
    double average_memory, double sigma_memory,
    double average_infile_size, double sigma_infile_size,
    double average_outfile_size, double sigma_outfile_size,
    const bool use_blockstreaming = false,
    const bool use_simplified_blockstreaming = true,
    double xrd_block_size = 1*1000*1000*1000,
    const double dummy_flops = std::numeric_limits<double>::min()
) {
    // Initialize random number generators
    std::normal_distribution<> flops(average_flops, sigma_flops);
    std::normal_distribution<> mem(average_memory, sigma_memory);
    std::normal_distribution<> insize(average_infile_size, sigma_infile_size);
    std::normal_distribution<> outsize(average_outfile_size,sigma_outfile_size);

    for (size_t j = 0; j < num_jobs; j++) {
        // Sample strictly positive task flops
        double dflops = flops(gen);
        while ((average_flops+sigma_flops) < dflops || dflops < 0.) dflops = flops(gen);
        // Sample strictly positive task memory requirements
        double dmem = mem(gen);
        while ((average_memory+sigma_memory) < dmem || dmem < 0.) dmem = mem(gen);

        // Connect the chains spanning all input-files of a job
        wrench::WorkflowTask* endtask = nullptr;
        wrench::WorkflowTask* enddummytask = nullptr;
        // when blockstreaming is turned off create only one task with all inputfiles
        if (!use_blockstreaming) {
            endtask = workflow->addTask("task_"+std::to_string(j), dflops, 1, 1, dmem);
        }
        for (size_t f = 0; f < infiles_per_task; f++) {
            // Sample inputfile sizes
            double dinsize = insize(gen);
            while ((average_infile_size+sigma_infile_size) < dinsize || dinsize < 0.) dinsize = insize(gen); 
            
            // when blockstreaming is turned off create only one task with all inputfiles
            if (!use_blockstreaming) {
                endtask->addInputFile(workflow->addFile("infile_"+std::to_string(j)+"_file_"+std::to_string(f), dinsize));
                continue;
            }

            // when simplified blockstreaming is turned on create only one dummytask and task per infile
            if (use_simplified_blockstreaming) {
                xrd_block_size = dinsize;
            }    
            // Chunk inputfiles into blocks and create blockwise tasks and dummy tasks
            // chain them as sketched in https://github.com/HerrHorizontal/DistCacheSim/blob/test/sgbatch/Sketches/Task_streaming_idea.pdf to enable task streaming
            size_t nblocks = static_cast<size_t>(dinsize/xrd_block_size);
            wrench::WorkflowTask* dummytask_parent = nullptr;
            wrench::WorkflowTask* task_parent = nullptr;
            if (enddummytask && endtask) {
                // Connect the chain to the previous input-file's
                dummytask_parent = enddummytask;
                task_parent = endtask;
            }
            else if (endtask) {
                throw std::runtime_error("There is no matching enddummytask for endtask "+endtask->getID());
            }
            else if (enddummytask) {
                throw std::runtime_error("There is no matching endtask for enddummytask "+enddummytask->getID());
            }
            for (size_t b = 0; b < nblocks; b++) {
                // Dummytask with inputblock and previous dummytask dependence
                // with minimal number of memory and flops 
                auto dummytask = workflow->addTask("dummytask_"+std::to_string(j)+"_file_"+std::to_string(f)+"_block_"+std::to_string(b), dummy_flops, 1, 1, dummy_flops);
                double blocksize = xrd_block_size;
                dummytask->addInputFile(workflow->addFile("infile_"+std::to_string(j)+"_file_"+std::to_string(f)+"_block_"+std::to_string(b), blocksize));
                if (dummytask_parent) {
                    workflow->addControlDependency(dummytask_parent, dummytask);
                }
                dummytask_parent = dummytask;
                // Task with dummytask and previous task dependence
                double blockflops = dflops * blocksize/dinsize;
                auto task = workflow->addTask("task_"+std::to_string(j)+"_file_"+std::to_string(f)+"_block_"+std::to_string(b), blockflops, 1, 1, dmem);
                workflow->addControlDependency(dummytask, task);
                if (task_parent) {
                    workflow->addControlDependency(task_parent, task);
                }
                task_parent = task;
                // Last blocktask is endtask
                if (b == nblocks-1) {
                    enddummytask = dummytask;
                    endtask = task;
                }
            }
            // when the input-file size is not an integer multiple of the XRootD blocksize create a last block task which takes care of the modulo
            // when blockwise streaming is turned off this evaluates to false
            if (double blocksize = (dinsize - nblocks*xrd_block_size)) {
                auto dummytask = workflow->addTask("dummytask_"+std::to_string(j)+"_file_"+std::to_string(f)+"_block_"+std::to_string(nblocks), dummy_flops, 1, 1, dummy_flops);
                dummytask->addInputFile(workflow->addFile("infile_"+std::to_string(j)+"_file_"+std::to_string(f)+"_block_"+std::to_string(nblocks), blocksize));
                if (dummytask_parent) {
                    workflow->addControlDependency(dummytask_parent, dummytask);
                }
                double blockflops = dflops * blocksize/dinsize;
                auto task = workflow->addTask("task_"+std::to_string(j)+"_file_"+std::to_string(f)+"_block_"+std::to_string(nblocks), blockflops, 1, 1, dmem);
                workflow->addControlDependency(dummytask, task);
                if (task_parent) {
                    workflow->addControlDependency(task_parent, task);
                }
                enddummytask = dummytask;
                endtask = task;
            }
        }

        // Sample outfile sizes
        double doutsize = outsize(gen);
        while ((average_outfile_size+sigma_outfile_size) < doutsize || doutsize < 0.) doutsize = outsize(gen); 
        endtask->addOutputFile(workflow->addFile("outfile_"+std::to_string(j), doutsize));
        //TODO: test if the complete chain has the right amount of tasks and dummytasks
    }
}


int main(int argc, char **argv) {

    // Declaration of the top-level WRENCH simulation object
    auto simulation = new wrench::Simulation();

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

    // Flags to turn on/off blockwise streaming of input-files
    bool use_blockstreaming = vm["blockstreaming"].as<bool>();
    bool use_simplified_blockstreaming = vm["simplified-blockstreaming"].as<bool>();


    /* Create a workflow */
    std::cerr << "Loading workflow..." << std::endl;
    auto workflow = new wrench::Workflow();
    
    fill_streaming_workflow(
        workflow, 
        num_jobs, infiles_per_job,
        average_flops, sigma_flops,
        average_memory,sigma_memory,
        average_infile_size, sigma_infile_size,
        average_outfile_size, sigma_outfile_size,
        use_blockstreaming,
        use_simplified_blockstreaming
    );

    std::cerr << "The workflow has " << workflow->getNumberOfTasks() << " tasks in " << std::to_string(num_jobs) << " chains" << std::endl;


    /* Read and parse the platform description file to instantiate a simulation platform */
    std::cerr << "Instantiating SimGrid platform..." << std::endl;
    simulation->instantiatePlatform(platform_file);


    /* Create storage and compute services and add them to the simulation */ 
    // Loop over vector of all the hosts in the simulated platform
    std::vector<std::string> hostname_list = simulation->getHostnameList();
    // Create a list of storage services that will be used by the WMS
    std::set<std::shared_ptr<wrench::StorageService>> storage_services;
    // Split into cache storages
    std::set<std::shared_ptr<wrench::StorageService>> cache_storage_services;
    // and a remote storage that is able to serve all file requests
    std::set<std::shared_ptr<wrench::StorageService>> remote_storage_services;
    // Create a list of compute services that will be used by the HTCondorService
    std::set<std::shared_ptr<wrench::ComputeService>> condor_compute_resources;
    std::string wms_host = "WMSHost";
    for (std::vector<std::string>::iterator hostname = hostname_list.begin(); hostname != hostname_list.end(); ++hostname) {
        std::string hostname_transformed = *hostname;
        std::for_each(hostname_transformed.begin(), hostname_transformed.end(), [](char& c){c = std::tolower(c);});
        // Instantiate storage services
        // WMSHost doesn't need a StorageService
        if (*hostname != wms_host) {
            std::string storage_host = *hostname;
            std::cerr << "Instantiating a SimpleStorageService on " << storage_host << "..." << std::endl;
            auto storage_service = simulation->add(new wrench::SimpleStorageService(storage_host, {"/"}));
            if (hostname_transformed.find("remote") != std::string::npos) {
                remote_storage_services.insert(storage_service);
            } else {
                cache_storage_services.insert(storage_service);
            }
            storage_services.insert(storage_service);
        } 
        // Instantiate bare-metal compute-services
        if (
            (*hostname != wms_host) && 
            (hostname_transformed.find("storage") == std::string::npos)
        ) {
            condor_compute_resources.insert(
                simulation->add(
                    new wrench::BareMetalComputeService(
                        *hostname,
                        {std::make_pair(
                            *hostname,
                            std::make_tuple(
                                wrench::Simulation::getHostNumCores(*hostname),
                                wrench::Simulation::getHostMemoryCapacity(*hostname)
                            )
                        )},
                        ""
                    )
                )
            );
        }
    }
    // Instantiate a HTcondorComputeService and add it to the simulation
    std::set<shared_ptr<wrench::ComputeService>> htcondor_compute_services;
    htcondor_compute_services.insert(shared_ptr<wrench::ComputeService>(simulation->add(
        new wrench::HTCondorComputeService(
            wms_host,
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
    )));


    /* Instantiate a file registry service */
    std::string file_registry_service_host = wms_host;
    std::cerr << "Instantiating a FileRegistryService on " << file_registry_service_host << "..." << std::endl;
    auto file_registry_service =
                    simulation->add(new wrench::FileRegistryService(file_registry_service_host));


    /* Instantiate a WMS */
    auto wms = simulation->add(
                    new SimpleWMS(
                        htcondor_compute_services, 
                        //TODO: at this point only remote storage services should be sufficient
                        storage_services,
                        wms_host,
                        //hitrate,
                        filename
                    )
    );
    wms->addWorkflow(workflow);


    /* Instatiate inputfiles */
    // Check that the right remote_storage_service is passed for initial inputfile storage
    // TODO: generalize to arbitrary numbers of remote storages
    if (remote_storage_services.size() != 1) {
        throw std::runtime_error("This example Simple Simulator requires a single remote_storage_service");
    }
    auto remote_storage_service = *remote_storage_services.begin();

    // It is necessary to store, or "stage", input files (blocks)
    std::cerr << "Staging input files..." << std::endl;
    std::vector<wrench::WorkflowTask*> tasks = workflow->getTasks();
    try {
        for (auto task : tasks) {
            auto input_files = task->getInputFiles();
            // Shuffle the input files
            std::shuffle(input_files.begin(), input_files.end(), gen);
            // Compute the task's incremental inputfiles size
            double incr_inputfile_size = 0.;
            for (auto f : input_files) {
                incr_inputfile_size += f->getSize();
            }
            // Distribute the infiles on all caches untill desired hitrate is reached
            double cached_files_size = 0.;
            for (auto const &f : input_files) {
                simulation->stageFile(f, remote_storage_service);
                if (cached_files_size < hitrate*incr_inputfile_size) {
                    for (auto cache : cache_storage_services) {
                        simulation->stageFile(f, cache);
                    }
                    cached_files_size += f->getSize();
                }
            }
            if (cached_files_size/incr_inputfile_size < hitrate) {
                throw std::runtime_error("Desired hitrate was not reached!");
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

