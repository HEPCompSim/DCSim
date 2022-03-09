

#ifndef S_COPYCOMPUTATION_H
#define S_COPYCOMPUTATION_H

#include <wrench-dev.h>

#include "CacheComputation.h"

class CopyComputation : public CacheComputation {

public:
    // TODO: REMOVE MOST THINGS IN HERE AND RELY ON THE GLOBALS IN SimpleSimulation::...
    CopyComputation(
        std::set<std::shared_ptr<wrench::StorageService>> &cache_storage_services,
        std::set<std::shared_ptr<wrench::StorageService>> &grid_storage_services,
        std::vector<std::shared_ptr<wrench::DataFile>> &files,
        double total_flops
    );

    void performComputation(std::string &hostname) override;

private:

};

#endif //S_COPYCOMPUTATION_H
