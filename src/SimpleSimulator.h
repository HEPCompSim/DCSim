

#ifndef S_SIMPLESIMULATOR_H
#define S_SIMPLESIMULATOR_H

#include "LRU_FileList.h"
#include "Workload.h"
#include "Dataset.h"

class SimpleSimulator {

public:
    static void identifyHostTypes(const std::shared_ptr<wrench::Simulation> &simulation);

    static std::set<std::string> cache_hosts;     // hosts configured to provide a cache
    static std::set<std::string> storage_hosts;   // hosts configured to provide GRID storage
    static std::set<std::string> worker_hosts;    // hosts configured to provide worker capacity
    static std::set<std::string> scheduler_hosts; // hosts configured to provide HTCondor scheduler
    static std::set<std::string> executors;       // hosts configured to provide manage job activities
    static std::set<std::string> file_registries; // hosts configured to manage a file registry
    static std::set<std::string> network_monitors;// hosts configured to monitor network

    static void fillHostsInSiblingZonesMap(bool include_subzones);

    static std::map<std::string, std::set<std::string>> hosts_in_zones;// map holding information of all hosts present in network zones
    static std::map<std::shared_ptr<wrench::StorageService>, LRU_FileList> global_file_map;// map holding files informations

    // global simulator settings and parameters
    static bool infile_caching_on;
    static bool prefetching_on;
    static bool local_cache_scope;

    static bool shuffle_jobs;

    static sg_size_t xrd_block_size;
    static double xrd_add_flops_per_time;
    static double xrd_add_flops_local_per_time;
    
    static std::mt19937 gen;

    /*// Cores required
    static int req_cores;
    // Flops distribution
    static double mean_flops;
    static double sigma_flops;
    static std::normal_distribution<double> *flops_dist;
    // Memory distribution
    static double mean_mem;
    static double sigma_mem;
    static std::normal_distribution<double> *mem_dist;
    // Input-file distribution
    static double mean_insize;
    static double sigma_insize;
    static std::normal_distribution<double> *insize_dist;
    // Output-file distribution
    static double mean_outsize;
    static double sigma_outsize;
    static std::normal_distribution<double> *outsize_dist;*/

    /** @brief Output filestream object to write out dump */
    static std::ofstream filedump;
};

#endif//S_SIMPLESIMULATOR_H
