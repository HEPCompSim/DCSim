

#ifndef S_COPYCOMPUTATION_H
#define S_COPYCOMPUTATION_H

#include <wrench-dev.h>

#include "CacheComputation.h"

class CopyComputation : public CacheComputation {

public:
    CopyComputation(
            const std::set<std::shared_ptr<wrench::StorageService>> &cache_storage_services,
            const std::set<std::shared_ptr<wrench::StorageService>> &grid_storage_services,
            const std::vector<std::shared_ptr<wrench::DataFile>> &files,
            double total_flops);

    void performComputation(const std::shared_ptr<wrench::ActionExecutor> &action_executor) override;

private:
};

#endif//S_COPYCOMPUTATION_H
