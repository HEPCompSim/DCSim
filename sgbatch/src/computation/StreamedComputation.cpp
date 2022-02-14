#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(streamed_computation, "Log category for StreamedComputation");

#include "StreamedComputation.h"


StreamedComputation::StreamedComputation(std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                         std::vector<std::shared_ptr<wrench::DataFile>> &files,
                                         double total_flops) : CacheComputation::CacheComputation(
                                             storage_services,
                                             files,
                                             total_flops
                                         ) {}

void StreamedComputation::performComputation(std::string &hostname) {
    WRENCH_INFO("Performing streamed computation!");
    // Incremental size of all input files to be processed
    auto total_data_size = this->total_data_size;
    for (auto const &fs : this->file_sources) {
        WRENCH_INFO("Streaming computation for input file %s", fs.first->getID().c_str());
        double data_to_process = fs.first->getSize();

        // Compute the number of blocks
        int num_blocks = int(std::ceil(data_to_process / (double) SimpleSimulator::xrd_block_size));

        // Read the first block
        fs.second->getStorageService()->readFile(fs.first, fs.second, std::min<double>(SimpleSimulator::xrd_block_size, data_to_process));

        // Process next blocks: compute block i while reading block i+i
        for (int i=0; i < num_blocks - 1; i++) {
            double num_bytes = std::min<double>(SimpleSimulator::xrd_block_size, data_to_process);
            double num_flops = determineFlops(num_bytes, total_data_size);
//            WRENCH_INFO("Chunk: %.2lf bytes / %.2lf flops", num_bytes, num_flops);
            // Start the computation asynchronously
            simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(num_flops);
            exec->start();
            // Read data from the file
            fs.second->getStorageService()->readFile(fs.first, fs.second, num_bytes);
            // Wait for the computation to be done
            exec->wait();
            data_to_process -= num_bytes;
        }

        // Process last block
        double num_flops = determineFlops(std::min<double>(SimpleSimulator::xrd_block_size, data_to_process), total_data_size);
        simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(num_flops);
        exec->start();
        exec->wait();

    }

}

