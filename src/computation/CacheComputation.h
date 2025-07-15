

#ifndef S_CACHECOMPUTATION_H
#define S_CACHECOMPUTATION_H

#include <wrench-dev.h>

#include "../SimpleSimulator.h"

class CacheComputation {

public:
    CacheComputation(
            std::set<std::shared_ptr<wrench::StorageService>> &cache_storage_services,
            std::set<std::shared_ptr<wrench::StorageService>> &grid_storage_services,
            std::vector<std::shared_ptr<wrench::DataFile>> &files,
            double total_flops);

    virtual ~CacheComputation() = default;

    void determineFileSourcesAndCache(std::shared_ptr<wrench::ActionExecutor> action_executor, bool cache_files);

    void operator()(std::shared_ptr<wrench::ActionExecutor> action_executor);

    double determineFlops(sg_size_t data_size, sg_size_t total_data_size) const;

    virtual void performComputation(std::shared_ptr<wrench::ActionExecutor> action_executor) = 0;

protected:
    std::set<std::shared_ptr<wrench::StorageService>> cache_storage_services;
    std::set<std::shared_ptr<wrench::StorageService>> grid_storage_services;
    std::vector<std::shared_ptr<wrench::DataFile>> files;//? does this need to be ordered?
    double total_flops;

    std::vector<std::pair<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>> file_sources;

    sg_size_t determineTotalDataSize(const std::vector<std::shared_ptr<wrench::DataFile>> &files);
    sg_size_t total_data_size;
};

#endif//S_CACHECOMPUTATION_H
