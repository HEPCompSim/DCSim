#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(copy_computation, "Log category for CopyComputation");

#include "CopyComputation.h"

CopyComputation::CopyComputation(std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                         std::vector<std::shared_ptr<wrench::DataFile>> &files,
                                         double total_flops) : CacheComputation::CacheComputation(
                                             storage_services,
                                             files,
                                             total_flops
                                         ) {}

void CopyComputation::performComputation(std::string &hostname) {
    WRENCH_INFO("Performing copy computation!");
    // Incremental size of all input files to process
    double total_data_size = this->total_data_size;
    // Read all input files before computation
    double data_size = 0;
    for (auto const &fs : this->file_sources) {
        WRENCH_INFO("Reading file %s from storage service on host %s",
                    fs.first->getID().c_str(), fs.second->getStorageService()->getHostname().c_str());
        fs.second->getStorageService()->readFile(fs.first, fs.second);
        data_size += fs.first->getSize();
    }
    if (data_size != total_data_size) {
        throw std::runtime_error("Something went wrong in the data size computation!");
    }
    // Perform the computation as needed
    double flops = determineFlops(data_size, total_data_size);
    WRENCH_INFO("Computing %.2lf flops", flops);
    wrench::Simulation::compute(flops);
}

