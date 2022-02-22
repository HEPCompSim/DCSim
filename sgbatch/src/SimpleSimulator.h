

#ifndef S_SIMPLESIMULATOR_H
#define S_SIMPLESIMULATOR_H

#include "LRU_FileList.h"

class SimpleSimulator {

public:
    std::set<std::shared_ptr<wrench::StorageService>> storage_services;

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
