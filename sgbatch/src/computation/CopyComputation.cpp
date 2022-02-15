#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(copy_computation, "Log category for CopyComputation");

#include "CopyComputation.h"

/**
 * @brief Construct a new CopyComputation::CopyComputation object
 * to be used within a compute action, which shall take caching of input-files into account.
 * File read of all input-files and compute steps are performed sequentially.
 * 
 * @param storage_services Storage services reachable to retrieve input files (caches plus remote)
 * @param files Input files of the job to process
 * @param total_flops Total #FLOPS of the whole compute action of the job
 */
CopyComputation::CopyComputation(std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                         std::vector<std::shared_ptr<wrench::DataFile>> &files,
                                         double total_flops) : CacheComputation::CacheComputation(
                                             storage_services,
                                             files,
                                             total_flops
                                         ) {}

/**
 * @brief Perform the computation within the simulation of the job.
 * First read all input-files and then compute the whole number of FLOPS.
 * 
 * @param hostname DEPRECATED: Actually not needed anymore
 */
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
    if (! (std::abs(data_size-total_data_size) < 1.)) {
        throw std::runtime_error("Something went wrong in the data size computation!");
    }
    // Perform the computation as needed
    double flops = determineFlops(data_size, total_data_size);
    WRENCH_INFO("Computing %.2lf flops", flops);
    wrench::Simulation::compute(flops);
}

