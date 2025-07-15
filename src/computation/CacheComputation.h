

#ifndef S_CACHECOMPUTATION_H
#define S_CACHECOMPUTATION_H

#include <wrench-dev.h>

#include "../SimpleSimulator.h"

class CacheComputation {

public:
    CacheComputation(
            const std::set<std::shared_ptr<wrench::StorageService>> &cache_storage_services,
            const std::set<std::shared_ptr<wrench::StorageService>> &grid_storage_services,
            const std::vector<std::shared_ptr<wrench::DataFile>> &files,
            double total_flops);

    virtual ~CacheComputation() = default;

    void determineFileSourcesAndCache(const std::shared_ptr<wrench::ActionExecutor>& action_executor, bool cache_files);

    void operator()(const std::shared_ptr<wrench::ActionExecutor> &action_executor);

    [[nodiscard]] double determineFlops(sg_size_t data_size, sg_size_t total_data_size) const;

    virtual void performComputation(const std::shared_ptr<wrench::ActionExecutor> &action_executor);

protected:
    std::set<std::shared_ptr<wrench::StorageService>> cache_storage_services;
    std::set<std::shared_ptr<wrench::StorageService>> grid_storage_services;
    std::vector<std::shared_ptr<wrench::DataFile>> files;//? does this need to be ordered?
    double total_flops;

    std::vector<std::pair<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>> file_sources;
    double total_flops_;

    sg_size_t determineTotalDataSize(const std::vector<std::shared_ptr<wrench::DataFile>> &files) const;
    sg_size_t total_data_size;
};

#endif//S_CACHECOMPUTATION_H
