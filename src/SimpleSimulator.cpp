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
#include "WorkloadExecutionController.h"
#include "JobSpecification.h"

#include "util/Utils.h"

#include <iostream>
#include <fstream>

#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>


namespace po = boost::program_options;

/**
 *
 * "Global" static variables. Some here are a bit ugly of course, but they should help
 * with memory footprint by avoiding passing around / storing items that apply to
 * all jobs.
 */
const std::vector<std::string> dataset_keys = {
        "location", "num_files",
        "filesize"};
const std::vector<std::string> mandatory_workload_keys = {
        "num_jobs",
        "flops", "memory", "outfilesize",
        "workload_type", "submission_time"};
const std::vector<std::string> elective_workload_keys = {
        "infiles_per_job",
        "infile_dataset",
};
std::map<std::shared_ptr<wrench::StorageService>, LRU_FileList> SimpleSimulator::global_file_map;
std::mt19937 SimpleSimulator::gen(42);                           // random number generator
std::ofstream filedump;                                          // output file stream to write monitoring dump to
bool SimpleSimulator::infile_caching_on = true;                  // flag to turn off/on the caching of job input-files
bool SimpleSimulator::prefetching_on = true;                     // flag to enable prefetching during streaming
bool SimpleSimulator::shuffle_jobs = false;                      // flag to enable job shuffling during submission
double SimpleSimulator::xrd_block_size = 1. * 1000 * 1000 * 1000;// maximum size of the streamed file blocks in bytes for the XRootD-ish streaming
// TODO: The initialized below is likely bogus (at compile time?)
std::set<std::string> SimpleSimulator::cache_hosts;
std::set<std::string> SimpleSimulator::storage_hosts;
std::set<std::string> SimpleSimulator::worker_hosts;
std::set<std::string> SimpleSimulator::scheduler_hosts;
std::set<std::string> SimpleSimulator::executors;
std::set<std::string> SimpleSimulator::file_registries;
std::set<std::string> SimpleSimulator::network_monitors;
std::map<std::string, std::set<std::string>> SimpleSimulator::hosts_in_zones;
bool SimpleSimulator::local_cache_scope = false;// flag to consider only local caches


/**
 * @brief Simple Choices class for cache scope program option
 * used as Custom Validator: https://www.boost.org/doc/libs/1_48_0/doc/html/program_options/howto.html#id2445062
 */
struct cacheScope {
    cacheScope(std::string const &val) : value(val) {}
    std::string value;
};
/**
 * @brief Operator<< for the cacheScope class
 * 
 * @param os 
 * @param val 
 * @return std::ostream& 
 */
std::ostream &operator<<(std::ostream &os, const cacheScope &val) {
    os << val.value << " ";
    return os;
}

/**
 * @brief Overload of boost::program_options validate method
 * to check for custom validator classes
 */
void validate(boost::any &v, std::vector<std::string> const &values, cacheScope * /* target_type */, int) {
    using namespace boost::program_options;

    // Make sure no previous assignment to 'v' was made.
    validators::check_first_occurrence(v);

    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    std::string const &s = validators::get_single_string(values);

    if (s == "local" || s == "network" || s == "siblingnetwork") {
        v = boost::any(cacheScope(s));
    } else {
        throw validation_error(validation_error::invalid_option_value);
    }
}

/**
 * @brief Simple Choices class for workload type program option
 * used as Custom Validator: https://www.boost.org/doc/libs/1_48_0/doc/html/program_options/howto.html#id2445062
 */
struct WorkloadTypeStruct {
    WorkloadTypeStruct(std::string const &val) : value(boost::to_lower_copy(val)) {}
    std::string value;
    // getter function
    WorkloadType get() const {
        return get_workload_type(value);
    }
};

/**
 * @brief Operator<< for the WorkloadTypeStruct class
 * 
 * @param os 
 * @param val 
 * @return std::ostream& 
 */
std::ostream &operator<<(std::ostream &os, const WorkloadTypeStruct &val) {
    os << val.value << " ";
    return os;
}

/**
 * @brief Overload of boost::program_options validate method
 * to check for custom validator classes
 */
void validate(boost::any &v, std::vector<std::string> const &values, WorkloadTypeStruct * /* target_type */, int) {
    using namespace boost::program_options;

    // Make sure no previous assignment to 'v' was made.
    validators::check_first_occurrence(v);

    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    std::string const &s = validators::get_single_string(values);

    auto w = WorkloadTypeStruct(s);
    try {
        w.get();
        v = boost::any(w);
    } catch (std::runtime_error &e) {
        throw validation_error(validation_error::invalid_option_value);
    }
}

/**
 * @brief Simple Choices class for workload type program option
 * used as Custom Validator: https://www.boost.org/doc/libs/1_48_0/doc/html/program_options/howto.html#id2445062
 */
struct StorageServiceBufferValue {
    StorageServiceBufferValue(std::string const &val) : value(boost::to_lower_copy(val)) {}
    std::string value;
    StorageServiceBufferType type;
    // getter function
    StorageServiceBufferType getType() const {
        return get_ssbuffer_type(value);
    }
    std::string get() const {
        return value;
    }
};

/**
 * @brief Operator<< for the StorageServiceBufferValue class
 * 
 * @param os 
 * @param val 
 * @return std::ostream& 
 */
std::ostream &operator<<(std::ostream &os, const StorageServiceBufferValue &val) {
    os << val.value << " ";
    return os;
}

/**
 * @brief Overload of boost::program_options validate method
 * to check for custom validator classes
 */
void validate(boost::any &v, std::vector<std::string> const &values, StorageServiceBufferValue * /* target_type */, int) {
    using namespace boost::program_options;

    // Make sure no previous assignment to 'v' was made.
    validators::check_first_occurrence(v);

    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    std::string const &s = validators::get_single_string(values);

    auto ssp = StorageServiceBufferValue(s);
    StorageServiceBufferType stype;
    try {
        stype = ssp.getType();
        // Ensure that non-value options are parsed correctly
        if (stype == StorageServiceBufferType::Zero) {
            v = boost::any(StorageServiceBufferValue("0"));
        } else if (stype == StorageServiceBufferType::Infinity) {
            v = boost::any(StorageServiceBufferValue("infinity"));
        } else {
            v = boost::any(ssp);
        }
    } catch (std::runtime_error &e) {
        throw validation_error(validation_error::invalid_option_value);
    }
}

/**
 * @brief helper function to process simulation options and parameters
 * 
 * @param argc
 * @param argv 
 * 
 */
po::variables_map process_program_options(int argc, char **argv) {

    // default values
    double hitrate = 0.0;

    double average_flops = 2164.428 * 1000 * 1000 * 1000;
    double sigma_flops = 0.1 * average_flops;
    double average_memory = 2. * 1000 * 1000 * 1000;
    double sigma_memory = 0.1 * average_memory;
    size_t infiles_per_job = 10;
    double average_infile_size = 3600000000.;
    double sigma_infile_size = 0.1 * average_infile_size;
    double average_outfile_size = 0.5 * infiles_per_job * average_infile_size;
    double sigma_outfile_size = 0.1 * average_outfile_size;


    size_t duplications = 1;

    bool no_caching = false;
    bool prefetch_off = false;
    bool shuffle_jobs = false;

    double xrd_block_size = 1000. * 1000 * 1000;
    std::string storage_service_buffer_size = "1048576";// 1MiB

    po::options_description desc("Allowed options");
    desc.add_options()// clang-format off
        ("help,h", "show brief usage message\n")

        ("platform,p", po::value<std::string>()->value_name("<platform>")->required(), "platform description file, written in XML following the SimGrid-defined DTD")
        ("hitrate,H", po::value<double>()->default_value(hitrate), "initial fraction of staged input-files on caches at simulation start")

        ("workload-configurations", po::value<std::vector<std::string>>()->multitoken()->default_value(std::vector<std::string>{}, ""), "List of paths to .json files with workload configurations. Note that all job-specific commandline options will be ignored in case at least one configuration is provided.")
        ("dataset-configurations", po::value<std::vector<std::string>>()->multitoken()->default_value(std::vector<std::string>{}, ""), "List of paths to .json files with dataset configurations.")

        ("duplications,d", po::value<size_t>()->default_value(duplications), "number of duplications of the workload to feed into the simulation")

        ("no-caching", po::bool_switch()->default_value(no_caching), "switch to turn on/off the caching of jobs' input-files")
        ("prefetch-off", po::bool_switch()->default_value(prefetch_off), "switch to turn on/off prefetching for streaming of input-files")
        ("shuffle-jobs", po::bool_switch()->default_value(shuffle_jobs), "switch to turn on/off shuffling jobs during submission")

        ("output-file,o", po::value<std::string>()->value_name("<out file>")->required(), "path for the CSV file containing output information about the jobs in the simulation")

        ("xrd-blocksize,x", po::value<double>()->default_value(xrd_block_size), "size of the blocks XRootD uses for data streaming")
        ("storage-buffer-size,b", po::value<StorageServiceBufferValue>()->default_value(StorageServiceBufferValue(storage_service_buffer_size)), "buffer size used by the storage services when communicating data")

        ("cache-scope", po::value<cacheScope>()->default_value(cacheScope("local")), "Set the network scope in which caches can be found:\n local: only caches on same machine\n network: caches in same network zone\n siblingnetwork: also include caches in sibling networks")
    ;// clang-format on

    po::variables_map vm;
    po::store(
            po::parse_command_line(argc, argv, desc),
            vm);

    if (vm.count("help")) {
        std::cerr << desc << std::endl;
        exit(EXIT_SUCCESS);
    }

    try {
        po::notify(vm);
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl
                  << std::endl;
        std::cerr << desc << std::endl;
        exit(EXIT_FAILURE);
    }

    // Here, all options should be properly set
    std::cerr << "Using platform " << vm["platform"].as<std::string>() << std::endl;

    return vm;
}


/**
 * @brief Method to duplicate the jobs of a workload
 * 
 * @param workload Workload containing jobs to duplicate
 * @param duplications Number of duplications each job is duplicated
 * @return std::map<std::string, JobSpecification> 
 */
std::map<std::string, JobSpecification> duplicateJobs(std::map<std::string, JobSpecification> &workload, size_t duplications, std::set<std::shared_ptr<wrench::StorageService>> grid_storage_services) {
    size_t num_jobs = workload.size();
    std::map<std::string, JobSpecification> dupl_workload;
    std::cerr << "\tDuplicating workload " << &workload << " with " << std::to_string(num_jobs) << " jobs ";
    for (auto &job_spec: workload) {
        boost::smatch job_index_matches;
        boost::regex job_index_expression{"\\d+"};
        boost::regex_search(job_spec.first, job_index_matches, job_index_expression);
        for (size_t d = 0; d < duplications; d++) {
            size_t dup_index;
            std::stringstream job_index_sstream(job_index_matches[job_index_matches.size() - 1]);
            job_index_sstream >> dup_index;
            dup_index += num_jobs * d;
            std::string dupl_job_id = boost::replace_last_copy(job_spec.first, job_index_matches[job_index_matches.size() - 1], std::to_string(dup_index));
            JobSpecification dupl_job_specs = job_spec.second;
            if (d > 0) {
                dupl_job_specs.outfile = wrench::Simulation::addFile(boost::replace_last_copy(dupl_job_specs.outfile->getID(), job_index_matches[job_index_matches.size() - 1], std::to_string(dup_index)), dupl_job_specs.outfile->getSize());
                // TODO: Think of a better way to copy the outfile destination
                for (auto ss: grid_storage_services) {
                    dupl_job_specs.outfile_destination = wrench::FileLocation::LOCATION(ss, dupl_job_specs.outfile);
                    break;
                }
            }
            dupl_workload.insert(std::make_pair(dupl_job_id, dupl_job_specs));
        }
    }
    std::cerr << "-> New workload has " << dupl_workload.size() << " jobs\n";
    return dupl_workload;
}


/**
 * @brief Identify demanded services on hosts to run based on configured "type" property tag
 * 
 * @param simulation Simulation object with already instantiated hosts
 * 
 * @throw std::runtime_error, std::invalid_argument
 */
void SimpleSimulator::identifyHostTypes(std::shared_ptr<wrench::Simulation> simulation) {
    std::vector<std::string> hostname_list = simulation->getHostnameList();
    if (hostname_list.size() == 0) {
        throw std::runtime_error("Empty hostname list! Have you instantiated the platform already?");
    }
    for (const auto &hostname: hostname_list) {
        auto hostProperties = wrench::S4U_Simulation::getHostProperty(hostname, "type");
        bool validType = false;
        if (hostProperties == "") {
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
        //TODO: Check for invalid types
        // if (! validType) {
        //     throw std::runtime_error("Invalid type " + hostProperties + " configuration in host " + hostname + "!");
        // }
    }
}

/**
 * @brief  Method to be executed once at simulation start,
 * which finds all hosts in zone and all same level accopanying zones (siblings)
 * and fills them into static map.
 * @param include_subzones Flag to alse include all hosts in sibling subzones. Default: false
 */
void SimpleSimulator::fillHostsInSiblingZonesMap(bool include_subzones = false) {
    std::map<std::string, std::vector<std::string>> zones_in_zones = wrench::S4U_Simulation::getAllSubZoneIDsByZone();
    std::map<std::string, std::vector<std::string>> hostnames_in_zones = wrench::S4U_Simulation::getAllHostnamesByZone();
    std::map<std::string, std::set<std::string>> tmp_hosts_in_zones;

    if (include_subzones) {// include all hosts in child-zones into a hostname set
        for (const auto &zones_in_zone: zones_in_zones) {
            std::cerr << "Zone: " << zones_in_zone.first << std::endl;
            for (const auto &zone: zones_in_zone.second) {
                std::cerr << "\tSubzone: " << zone << std::endl;
                for (const auto &host: hostnames_in_zones[zone]) {
                    std::cerr << "\t\tHost: " << host << std::endl;
                    tmp_hosts_in_zones[zones_in_zone.first].insert(host);
                    tmp_hosts_in_zones[zone].insert(host);
                }
            }
        }
    } else {// just convert the vector of hostnames to set in map
        for (const auto &hostnamesByZone: hostnames_in_zones) {
            std::vector<std::string> hostnamesVec = hostnamesByZone.second;
            std::set<std::string> hostnamesSet(hostnamesVec.begin(), hostnamesVec.end());
            tmp_hosts_in_zones[hostnamesByZone.first] = hostnamesSet;
        }
    }
    // identify all sibling zones and append their hosts
    for (const auto &hosts_in_zone: tmp_hosts_in_zones) {
        std::string zone = hosts_in_zone.first;
        auto parent_zone = simgrid::s4u::Engine::get_instance()->netzone_by_name_or_null(zone)->get_parent();
        auto hosts = hosts_in_zone.second;
        for (const auto &sibling: parent_zone->get_children()) {
            auto hosts = tmp_hosts_in_zones[sibling->get_name()];
            SimpleSimulator::hosts_in_zones[zone].insert(hosts.begin(), hosts.end());
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

    size_t duplications = vm["duplications"].as<size_t>();

    double hitrate = vm["hitrate"].as<double>();

    std::vector<std::string> workload_configurations = vm["workload-configurations"].as<std::vector<std::string>>();

    // dataset-configurations name containing datasets infomration
    std::vector<std::string> dataset_configurations = vm["dataset-configurations"].as<std::vector<std::string>>();

    // Flags to turn on/off the caching of jobs' input-files
    SimpleSimulator::infile_caching_on = !(vm["no-caching"].as<bool>());

    // Flags to turn prefetching for streaming of input-files
    std::cerr << "Prefetching switch off?: " << vm["prefetch-off"].as<bool>() << std::endl;
    SimpleSimulator::prefetching_on = !(vm["prefetch-off"].as<bool>());

    // Flag to turn on shuffling of jobs
    std::cerr << "Job shuffling on?: " << vm["shuffle-jobs"].as<bool>() << std::endl;
    SimpleSimulator::shuffle_jobs = vm["shuffle-jobs"].as<bool>();

    // Set XRootD block size
    SimpleSimulator::xrd_block_size = vm["xrd-blocksize"].as<double>();

    // Set StorageService buffer size/type
    std::string buffer_size = vm["storage-buffer-size"].as<StorageServiceBufferValue>().get();

    // Choice of cache locality scope
    std::string scope_caches = vm["cache-scope"].as<cacheScope>().value;
    bool rec_netzone_caches = false;
    if (scope_caches.find("network") == std::string::npos) {
        SimpleSimulator::local_cache_scope = true;
    } else {
        if (scope_caches.find("sibling") != std::string::npos) {
            rec_netzone_caches = true;
        }
    }

    /* Create datasets */

    std::cerr << "Constructing dataset specifications..." << std::endl;

    std::vector<Dataset> dataset_specs = {};

    if (dataset_configurations.size() == 0) {
        //Default dataset config
    } else {
        for (auto &ds_confpath: dataset_configurations) {
            std::ifstream ds_conf(ds_confpath);
            nlohmann::json dss_json = nlohmann::json::parse(ds_conf);

            // Looping over the multiple workloads configured in the json file
            for (auto &ds: dss_json.items()) {

                // Checking json syntax to match workload spec
                for (auto &ds_key: dataset_keys) {
                    try {
                        if (!ds.value().contains(ds_key)) {
                            throw std::invalid_argument("ERROR: the dataset configuration " + ds_confpath + " must contain " + ds_key + " as information.");
                        }
                    } catch (std::invalid_argument &e) {
                        std::cerr << e.what() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
                std::vector<std::string> location{};
                if (ds.value()["location"].type() == nlohmann::json::value_t::string)
                    location = {ds.value()["location"]};
                else
                    location = ds.value()["location"].get<std::vector<std::string>>();
                dataset_specs.push_back(
                        Dataset(
                                // TODO: support simple strings when only one host is required as location
                                location,
                                ds.value()["num_files"],
                                ds.value()["filesize"],
                                ds.key(),
                                SimpleSimulator::gen));
                std::cerr << "\tDataset " << std::string(ds.key()) << " loaded" << std::endl;
            }
        }
    }
    std::cerr << "Created " << dataset_specs.size() << " unique datasets!"
              << "\n";


    /* Create a workload */
    std::cerr << "Constructing workload specification..." << std::endl;

    std::vector<Workload> workload_specs = {};
    try {
        if (workload_configurations.size() == 0)
            throw std::invalid_argument("ERROR: the workload configuration loaded is invalid or empty.");
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    for (auto &wf_confpath: workload_configurations) {
        std::ifstream wf_conf(wf_confpath);
        try {
            if (!wf_conf.is_open())
                throw std::runtime_error("File " + wf_confpath + " could not be opened!");
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            exit(EXIT_FAILURE);
        }

        nlohmann::json wfs_json = nlohmann::json::parse(wf_conf);

        // Looping over the multiple workloads configured in the json file
        for (auto &wf: wfs_json.items()) {
            // Checking json syntax to match workload spec
            for (auto &wf_key: mandatory_workload_keys) {
                try {
                    if (!wf.value().contains(wf_key)) {
                        throw std::invalid_argument("ERROR: the workload configuration " + wf_confpath + " must contain " + wf_key + " as information.");
                    }
                } catch (std::invalid_argument &e) {
                    std::cerr << e.what() << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            std::string workload_type_lower = boost::to_lower_copy(std::string(wf.value()["workload_type"]));
            if (workload_type_lower != "calculation") {
                std::vector<std::string> infile_datasets{};
                if (wf.value()["infile_datasets"].type() == nlohmann::json::value_t::string)
                    infile_datasets = {wf.value()["infile_datasets"]};
                else
                    infile_datasets = wf.value()["infile_datasets"].get<std::vector<std::string>>();
                workload_specs.push_back(
                        Workload(
                                wf.value()["num_jobs"],
                                wf.value()["cores"],
                                wf.value()["flops"], wf.value()["memory"],
                                wf.value()["outfilesize"],
                                get_workload_type(workload_type_lower), wf.key(),
                                wf.value()["submission_time"],
                                SimpleSimulator::gen,
                                infile_datasets));
            } else {
                workload_specs.push_back(
                        Workload(
                                wf.value()["num_jobs"],
                                wf.value()["cores"],
                                wf.value()["flops"], wf.value()["memory"],
                                wf.value()["outfilesize"],
                                get_workload_type(workload_type_lower), wf.key(),
                                wf.value()["submission_time"],
                                SimpleSimulator::gen));
            }
            std::cerr << "\tThe workload " << std::string(wf.key()) << " has " << wf.value()["num_jobs"] << " unique jobs" << std::endl;
        }
    }
    std::cerr << "Created " << workload_specs.size() << " unique workloads!"
              << "\n";

    /* Add infiles to worklaod */

    for (auto &ws: workload_specs) {
        if (ws.workload_type == WorkloadType::Calculation)
            continue;
        ws.assignFiles(dataset_specs);
    }


    /* Read and parse the platform description file to instantiate a simulation platform */
    std::cerr << "Instantiating SimGrid platform..." << std::endl;
    simulation->instantiatePlatform(platform_file);


    /* Identify demanded and create storage and compute services and add them to the simulation */
    SimpleSimulator::identifyHostTypes(simulation);

    // Fill reachable caches map
    if (rec_netzone_caches) {
        SimpleSimulator::fillHostsInSiblingZonesMap();
    } else {
        for (const auto &hostnamesByZone: wrench::S4U_Simulation::getAllHostnamesByZone()) {
            std::vector<std::string> hostnamesVec = hostnamesByZone.second;
            std::set<std::string> hostnamesSet(hostnamesVec.begin(), hostnamesVec.end());
            SimpleSimulator::hosts_in_zones[hostnamesByZone.first] = hostnamesSet;
        }
    }

    // Create a list of cache storage services
    std::set<std::shared_ptr<wrench::StorageService>> cache_storage_services;
    for (auto host: SimpleSimulator::cache_hosts) {
        //TODO: Support more than one type of cache mounted differently?
        //TODO: This might not be necessary since different cache layers are typically on different hosts
        auto storage_service = simulation->add(
                wrench::SimpleStorageService::createSimpleStorageService(
                        host, {"/"},
                        {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, buffer_size}},
                        {}));
        cache_storage_services.insert(storage_service);
    }

    // and remote storages that are able to serve all file requests

    std::set<std::shared_ptr<wrench::StorageService>> grid_storage_services;
    for (auto host: SimpleSimulator::storage_hosts) {
        auto storage_service = simulation->add(
                wrench::SimpleStorageService::createSimpleStorageService(
                        host, {"/"},
                        {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, buffer_size}},
                        {}));
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
                                                wrench::Simulation::getHostMemoryCapacity(host)))},
                                "")));
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
                                {{wrench::HTCondorComputeServiceProperty::NEGOTIATOR_OVERHEAD, "0.0"},
                                 {wrench::HTCondorComputeServiceProperty::GRID_PRE_EXECUTION_DELAY, "0.0"},
                                 {wrench::HTCondorComputeServiceProperty::GRID_POST_EXECUTION_DELAY, "0.0"},
                                 {wrench::HTCondorComputeServiceProperty::NON_GRID_PRE_EXECUTION_DELAY, "0.0"},
                                 {wrench::HTCondorComputeServiceProperty::NON_GRID_POST_EXECUTION_DELAY, "0.0"}},
                                {})));
    }


    /* Instantiate file registry services */
    std::set<std::shared_ptr<wrench::FileRegistryService>> file_registry_services;
    for (auto host: SimpleSimulator::file_registries) {
        std::cerr << "Instantiating a FileRegistryService on " << host << "..." << std::endl;
        auto file_registry_service = simulation->add(new wrench::FileRegistryService(host));
        file_registry_services.insert(file_registry_service);
    }


    /* Instantiate Execution Controllers */
    std::set<std::shared_ptr<WorkloadExecutionController>> workload_execution_controllers;
    //TODO: Think of a way to support more than one execution controller host
    if (SimpleSimulator::executors.size() != 1) {
        throw std::runtime_error("Currently this simulator supports only a single host running workload execution controllers!");
    }
    std::cerr << "Creating workload execution controllers..."
              << "\n";
    for (auto host: SimpleSimulator::executors) {
        for (auto &workload_spec: workload_specs) {
            auto wms = simulation->add(
                    new WorkloadExecutionController(
                            workload_spec,
                            htcondor_compute_services,
                            grid_storage_services,
                            cache_storage_services,
                            host,
                            filename,
                            SimpleSimulator::shuffle_jobs,
                            SimpleSimulator::gen));
            std::cerr << "\tCreated execution controller " << wms->getName() << " executing workload " << &workload_spec << " with " << workload_spec.job_batch.size() << " jobs to simulate\n";
            workload_execution_controllers.insert(wms);
        }
        std::cerr << "Total number of execution controllers: " << workload_execution_controllers.size() << "\n";
    }

    /* Instantiate inputfiles and set outfile destinations*/
    std::cerr << "Creating and staging input files" << std::endl;
    try {
        for (auto dss: dataset_specs) {
            std::shuffle(dss.files.begin(), dss.files.end(), SimpleSimulator::gen);
            //TODO: Add total_file_size as dataset property

            for (auto const &f: dss.files) {
                // Distribute the dataset files on specified GRID storages
                //TODO: Think of a more realistic distribution pattern and avoid duplications
                for (auto storage_service: grid_storage_services) {
                    if (std::find(dss.hostnames.begin(), dss.hostnames.end(), storage_service->getHostname()) == dss.hostnames.end())
                        continue;
                    simulation->stageFile(wrench::FileLocation::LOCATION(storage_service, f));
                    SimpleSimulator::global_file_map[storage_service].touchFile(f.get());
                }
            }
        }
        for (auto wms: workload_execution_controllers) {
            for (auto &job_spec: wms->get_workload_spec()) {
                double incr_infile_size = 0.;
                double cached_files_size = 0.;
                for (auto const &f: job_spec.second.infiles) {
                    incr_infile_size += f->getSize();
                }

                for (auto const &f: job_spec.second.infiles) {

                    // Distribute the files on all caches until desired hitrate is reached
                    // TODO: Rework the initialization of input files on caches
                    if (cached_files_size < hitrate * incr_infile_size) {
                        for (const auto &cache: cache_storage_services) {
                            // simulation->stageFile(f, cache);
                            simulation->stageFile(wrench::FileLocation::LOCATION(cache, f));
                            SimpleSimulator::global_file_map[cache].touchFile(f.get());
                        }
                        cached_files_size += f->getSize();
                    }
                }
                if (cached_files_size / incr_infile_size < hitrate) {
                    throw std::runtime_error("Desired hitrate was not reached!");
                }
            }
        }
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 0;
    }
    std::cerr << "Set destination of output files..." << std::endl;
    for (auto wms: workload_execution_controllers) {
        try {
            for (auto &job_spec: wms->get_workload_spec()) {
                // Set outfile destinations
                // TODO: Think of a way to identify a specific (GRID) storage
                for (auto storage_service: grid_storage_services) {
                    job_spec.second.outfile_destination = wrench::FileLocation::LOCATION(storage_service, job_spec.second.outfile);
                    break;
                }
            }
        } catch (std::runtime_error &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            return 0;
        }
    }

    std::cerr << "Duplicating workloads ... "
              << "\n";
    size_t num_total_jobs = 0;
    for (auto wms: workload_execution_controllers) {
        /* Duplicate the workload */
        auto new_workload_spec = duplicateJobs(wms->get_workload_spec(), duplications, grid_storage_services);
        wms->set_workload_spec(new_workload_spec);
        num_total_jobs += new_workload_spec.size();
    }
    std::cerr << "The simulation now has " << std::to_string(num_total_jobs) << " jobs in total " << std::endl;


    /* Launch the simulation */
    try {
        /* initialize output-dump file */
        filedump.open(filename, ios::out | ios::trunc);
        if (filedump.is_open()) {
            filedump << "job.tag"
                     << ", ";// << "job.ncpu" << ", " << "job.memory" << ", " << "job.disk" << ", ";
            filedump << "machine.name"
                     << ", ";
            filedump << "hitrate"
                     << ", ";
            filedump << "job.start"
                     << ", "
                     << "job.end"
                     << ", "
                     << "job.computetime"
                     << ", "
                     << "job.flops"
                     << ", ";
            filedump << "infiles.transfertime"
                     << ", "
                     << "infiles.size"
                     << ", "
                     << "outfiles.transfertime"
                     << ", "
                     << "outfiles.size"
                     << "\n";
            filedump.close();
            std::cerr << "Wrote header of the output dump into file " << filename << std::endl;
        } else {
            throw std::runtime_error("Couldn't open output-file " + filename + " for dump!");
        }
        std::cerr << "Launching the Simulation..." << std::endl;
        simulation->launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 0;
    }
    std::cerr << "Simulation done! " << wrench::Simulation::getCurrentSimulatedDate() << std::endl;

    // Check routes from workers to remote storages
#if 0
    for (auto worker_host_name: SimpleSimulator::worker_hosts) {
        for(auto remote_host_name: SimpleSimulator::storage_hosts) {
            std::vector<simgrid::s4u::Link*> links;
            double latency;
            auto worker_host = simgrid::s4u::Host::by_name(worker_host_name);
            auto remote_host = simgrid::s4u::Host::by_name(remote_host_name);
            worker_host->route_to(remote_host, links, &latency);
            std::cerr << "ROUTE FROM " << worker_host->get_name() << " TO " << remote_host->get_name() << ":\n";
            for (const auto l: links) {
                std::cerr << " - " << l->get_name() << "\n";
            }
        }
    }
#endif


    return 0;
}
