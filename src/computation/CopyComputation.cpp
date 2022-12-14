#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(copy_computation, "Log category for CopyComputation");

#include "CopyComputation.h"
#include "MonitorAction.h"

/**
 * @brief Construct a new CopyComputation::CopyComputation object
 * to be used as a lambda within a compute action, which shall take caching of input-files into account.
 * File read of all input-files and compute steps are performed sequentially.
 * 
 * @param storage_services Storage services reachable to retrieve input files (caches plus remote)
 * @param files Input files of the job to process
 * @param total_flops Total #FLOPS of the whole compute action of the job
 */
CopyComputation::CopyComputation(
    std::set<std::shared_ptr<wrench::StorageService>> &cache_storage_services,
    std::set<std::shared_ptr<wrench::StorageService>> &grid_storage_services,
    std::vector<std::shared_ptr<wrench::DataFile>> &files,
    double total_flops) : CacheComputation::CacheComputation(
        cache_storage_services,
        grid_storage_services,
        files,
        total_flops
    ) {}

/**
 * @brief Perform the computation within the simulation of the job.
 * First read all input-files and then compute the whole number of FLOPS.
 * 
 * @param action_executor Handle to access the action this computation belongs to
 */
void CopyComputation::performComputation(std::shared_ptr<wrench::ActionExecutor> action_executor) {

    auto the_action = std::dynamic_pointer_cast<MonitorAction>(action_executor->getAction()); // executed action

    double infile_transfer_time = 0.;
    double compute_time = 0.;

    WRENCH_INFO("Performing copy computation!");
    // Incremental size of all input files to process
    double total_data_size = this->total_data_size;
    // Read all input files before computation
    double data_size = 0;
    for (auto const &fs : this->file_sources) {
        WRENCH_INFO("Reading file %s from storage service on host %s",
                    fs.first->getID().c_str(), fs.second->getStorageService()->getHostname().c_str());

        double read_start_time = wrench::Simulation::getCurrentSimulatedDate();
        fs.second->getStorageService()->readFile(fs.second);
        double read_end_time = wrench::Simulation::getCurrentSimulatedDate();

        data_size += fs.first->getSize();
        if (read_end_time >= read_start_time) {
            infile_transfer_time += read_end_time - read_start_time;
        } else {
            throw std::runtime_error(
                "Reading file " + fs.first->getID() + " finished before it started!"
            );
        }
    }
    if (! (std::abs(data_size-total_data_size) < 1.)) {
        throw std::runtime_error("Something went wrong in the data size computation!");
    }

    // Perform the computation as needed
    double flops = determineFlops(data_size, total_data_size);
    WRENCH_INFO("Computing %.2lf flops", flops);
    double compute_start_time = wrench::Simulation::getCurrentSimulatedDate();
    wrench::Simulation::compute(flops);
    double compute_end_time = wrench::Simulation::getCurrentSimulatedDate();

    if (compute_end_time > compute_start_time) {
        compute_time += compute_end_time - compute_start_time;
    } else {
        throw std::runtime_error(
            "Computing job " + the_action->getJob()->getName() + " finished before it started!"
        );
    }

    // Fill monitoring information
    the_action->set_infile_transfer_time(infile_transfer_time);
    the_action->set_calculation_time(compute_time);
}

