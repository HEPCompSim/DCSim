

#ifndef S_SIMPLESIMULATOR_H
#define S_SIMPLESIMULATOR_H

#include "LRU_FileList.h"

class SimpleSimulator {

public:
    std::set<std::shared_ptr<wrench::StorageService>> storage_services;

    static bool use_blockstreaming;
    static bool use_simplified_blockstreaming;
    static std::map<std::shared_ptr<wrench::StorageService>, LRU_FileList> global_file_map;
    static double xrd_block_size;

    // Random number generator
    static std::mt19937 gen;

    // Flops distribution
    static double mean_flops_per_block;
    static double sigma_flops_per_block;
    static std::normal_distribution<double> *flops_per_block_dist;
};

#endif //S_SIMPLESIMULATOR_H
