

#ifndef S_STREAMEDCOMPUTATION_H
#define S_STREAMEDCOMPUTATION_H

#include <wrench-dev.h>

#include "CacheComputation.h"

class StreamedComputation : public CacheComputation {

public:
    // TODO: REMOVE MOST THINGS IN HERE AND RELY ON THE GLOBALS IN SimpleSimulation::...
    StreamedComputation(
        std::set<std::shared_ptr<wrench::StorageService>> &cache_storage_services,
        std::set<std::shared_ptr<wrench::StorageService>> &grid_storage_services,
        std::vector<std::shared_ptr<wrench::DataFile>> &files,
        double total_flops,
        bool prefetch_on
    );

    void performComputation(std::shared_ptr<wrench::ActionExecutor> action_executor) override;

private:

    bool prefetching_on;

};

#endif //S_STREAMEDCOMPUTATION_H
