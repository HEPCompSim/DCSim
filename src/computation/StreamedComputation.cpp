#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(streamed_computation, "Log category for StreamedComputation");

#include "StreamedComputation.h"
#include "MonitorAction.h"


/**
 * @brief Construct a new StreamedComputation::StreamedComputation object
 * to be used as a lambda within a compute action, which shall take caching of input-files into account.
 * File read is performed asynchronously in blocks and the corresponding compute step is executed
 * once the corresponding block is available.
 * 
 * @param cache_storage_services Storage services reachable to retrieve input files (caches)
 * @param grid_storage_services Storage services reachable to retrieve input files (remote)
 * @param files Input files of the job to process
 * @param total_flops Total #FLOPS of the whole compute action of the job
 * @param prefetch_on Whether pre-fetching is on or not
 */
StreamedComputation::StreamedComputation(
        std::set<std::shared_ptr<wrench::StorageService>> &cache_storage_services,
        const std::set<std::shared_ptr<wrench::StorageService>> &grid_storage_services,
        const std::vector<std::shared_ptr<wrench::DataFile>> &files,
        const double total_flops, const bool prefetch_on) : CacheComputation::CacheComputation(cache_storage_services,
                                                                                   grid_storage_services,
                                                                                   files,
                                                                                   total_flops) { prefetching_on = prefetch_on; }

/**
 * @brief Perform the computation within the simulation of the job.
 * Asynchronously read the input files (don't wait for previous computation to finish) in blocks 
 * and compute the according share of FLOPS once read finished.
 * 
 * @param action_executor Handle to access the action this computation belongs to
 */
void StreamedComputation::performComputation(const std::shared_ptr<wrench::ActionExecutor> &action_executor) {

    auto the_action = std::dynamic_pointer_cast<MonitorAction>(action_executor->getAction());// executed action

    double infile_transfer_time = 0.;
    double compute_time = 0.;

    WRENCH_INFO("Performing streamed computation!");
    // Incremental size of all input files to be processed
    auto total_data_size = this->total_data_size;
    for (auto const &fs: this->file_sources) {
        bool file_local = (fs.second->getStorageService()->getHostname() == wrench::Simulation::getHostName());
        WRENCH_INFO("Streaming computation for input file %s in location %s", fs.first->getID().c_str(), fs.second->getStorageService()->getHostname().c_str());
        sg_size_t data_to_process = fs.first->getSize();

        // Compute the number of blocks
        auto num_blocks = static_cast<int>(std::ceil(static_cast<double>(data_to_process) / static_cast<double>(SimpleSimulator::xrd_block_size)));

        // Read the first block
        double xrd_block_start_time;
        double read_start_time = wrench::Simulation::getCurrentSimulatedDate();
        fs.second->getStorageService()->readFile(fs.second, std::min<sg_size_t>(SimpleSimulator::xrd_block_size, data_to_process));
        double read_end_time = wrench::Simulation::getCurrentSimulatedDate();
        if (read_end_time > read_start_time) {
            infile_transfer_time += read_end_time - read_start_time;
            xrd_block_start_time = read_start_time;
            WRENCH_INFO("Streaming computation received block %d of file %s", 0, fs.first->getID().c_str());
        } else {
            throw std::runtime_error(
                    "Reading block " + std::to_string(0) +
                    " of file " + fs.first->getID() + " finished before it started!");
        }

        // Process next blocks: compute block i while reading block i+i
        for (int i = 0; i < num_blocks - 1; i++) {
            auto num_bytes = std::min<sg_size_t>(SimpleSimulator::xrd_block_size, data_to_process);
            double num_flops = determineFlops(num_bytes, total_data_size);
            WRENCH_INFO("Chunk: %llu bytes / %.2lf flops", num_bytes, num_flops);
            // Add XRootD FLOPs overhead that increments with execution time
            double xrd_overhead_flops = 0;
            if (!file_local) {
                xrd_overhead_flops = SimpleSimulator::xrd_add_flops_per_time * (wrench::Simulation::getCurrentSimulatedDate() - xrd_block_start_time);
            } else {
                xrd_overhead_flops = SimpleSimulator::xrd_add_flops_local_per_time * (wrench::Simulation::getCurrentSimulatedDate() - xrd_block_start_time);
            }
            num_flops += xrd_overhead_flops;
            WRENCH_DEBUG("       + %.2lf flops XRootD overhead", xrd_overhead_flops);
            // Start the computation asynchronously
            simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(num_flops);
            double exec_start_time = 0.0;
            double exec_end_time = 0.0;
            if (this->prefetching_on) {
                exec->start();
                exec_start_time = exec->get_start_time();
                // Read data from the file
                read_start_time = wrench::Simulation::getCurrentSimulatedDate();
                fs.second->getStorageService()->readFile(fs.second, num_bytes);
                read_end_time = wrench::Simulation::getCurrentSimulatedDate();
                // Wait for the computation to be done
                exec->wait();
                exec_end_time = exec->get_finish_time();
            } else {
                exec->start();
                exec_start_time = exec->get_start_time();
                exec->wait();
                exec_end_time = exec->get_finish_time();
                read_start_time = wrench::Simulation::getCurrentSimulatedDate();
                fs.second->getStorageService()->readFile(fs.second, num_bytes);
                read_end_time = wrench::Simulation::getCurrentSimulatedDate();
            }
            data_to_process -= num_bytes;
            if (exec_end_time >= exec_start_time) {
                compute_time += exec_end_time - exec_start_time;
                WRENCH_INFO("Streaming computation completed block %d of file %s", i, the_action->getJob()->getName().c_str());
            } else {
                throw std::runtime_error(
                        "Executing block " + std::to_string(i) +
                        " of job " + the_action->getJob()->getName() + " finished before it started!");
            }
            if (read_end_time > read_start_time) {
                infile_transfer_time += read_end_time - read_start_time;
                xrd_block_start_time = read_start_time;
                WRENCH_INFO("Streaming computation received block %d of file %s", i, fs.first->getID().c_str());
            } else {
                throw std::runtime_error(
                        "Reading block " + std::to_string(i) +
                        " of file " + fs.first->getID() + " finished before it started!");
            }
        }

        // Process last block
        double num_bytes = std::min<double>(SimpleSimulator::xrd_block_size, data_to_process);
        double num_flops = determineFlops(std::min<sg_size_t>(SimpleSimulator::xrd_block_size, data_to_process), total_data_size);
        WRENCH_INFO("Chunk: %.2lf bytes / %.2lf flops", num_bytes, num_flops);
        // Add XRootD FLOPs overhead that increments with execution time
        double xrd_overhead_flops = 0;
        if (!file_local) {
            xrd_overhead_flops = SimpleSimulator::xrd_add_flops_per_time * (wrench::Simulation::getCurrentSimulatedDate() - xrd_block_start_time);
        } else {
            xrd_overhead_flops = SimpleSimulator::xrd_add_flops_local_per_time * (wrench::Simulation::getCurrentSimulatedDate() - xrd_block_start_time);
        }
        num_flops += xrd_overhead_flops;
        WRENCH_DEBUG("       + %.2lf flops XRootD overhead", xrd_overhead_flops);
        simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(num_flops);
        exec->start();
        double exec_start_time = exec->get_start_time();
        exec->wait();
        double exec_end_time = exec->get_finish_time();
        if (exec_end_time >= exec_start_time) {
            compute_time += exec_end_time - exec_start_time;
            WRENCH_INFO("Streaming computation completed block %d of file %s", num_blocks - 1, the_action->getJob()->getName().c_str());
        } else {
            throw std::runtime_error(
                    "Executing block " + std::to_string(num_blocks - 1) +
                    " of job " + the_action->getJob()->getName() + " finished before it started!");
        }
    }

    // Fill monitoring information
    the_action->set_infile_transfer_time(infile_transfer_time);
    the_action->set_calculation_time(compute_time);
}
