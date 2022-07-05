

#ifndef S_SIMPLESIMULATOR_H
#define S_SIMPLESIMULATOR_H

#include "LRU_FileList.h"

class SimpleSimulator {

public:

    static void identifyHostTypes(std::shared_ptr<wrench::Simulation> simulation);

    static std::set<std::string> cache_hosts;       // hosts configured to provide a cache
    static std::set<std::string> storage_hosts;     // hosts configured to provide GRID storage
    static std::set<std::string> worker_hosts;      // hosts configured to provide worker capacity
    static std::set<std::string> scheduler_hosts;   // hosts configured to provide HTCondor scheduler
    static std::set<std::string> executors;         // hosts configured to provide manage job activities
    static std::set<std::string> file_registries;   // hosts configured to manage a file registry
    static std::set<std::string> network_monitors;  // hosts configured to monitor network

    static void fillHostsInRecZonesMap();

    static std::map<std::string, std::set<std::string>> hosts_in_rec_zones; // map holding information of all hosts present in network zones

    static bool use_blockstreaming;
    static std::map<std::shared_ptr<wrench::StorageService>, LRU_FileList> global_file_map;
    static double xrd_block_size;
    static std::mt19937 gen;

    // Flops distribution
    static double mean_flops;
    static double sigma_flops;
    static std::normal_distribution<double>* flops_dist;
    // Memory distribution
    static double mean_mem;
    static double sigma_mem;
    static std::normal_distribution<double>* mem_dist;
    // Input-file distribution
    static double mean_insize;
    static double sigma_insize;
    static std::normal_distribution<double>* insize_dist;
    // Output-file distribution
    static double mean_outsize;
    static double sigma_outsize;
    static std::normal_distribution<double>* outsize_dist;
};

#endif //S_SIMPLESIMULATOR_H
