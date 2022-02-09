#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(streamed_computation, "Log category for StreamedComputation");

#include "StreamedComputation.h"

StreamedComputation::StreamedComputation(std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                         std::vector<std::shared_ptr<wrench::DataFile>> &files,
                                         double total_flops) {
    this->storage_services = storage_services;
    this->files = files;
    this->total_flops = total_flops;
    this->total_data_size = determineTotalDataSize(files);

}

void StreamedComputation::determineFileSources(std::string hostname) {
    // Identify all storage services that run on this host, which runs the streaming action
    // TODO: HENRI QUESTION: IS IT REALLY THE CASE THERE ARE COULD BE MULTIPLE LOCAL STORAGE SERVICES???
    std::vector<std::shared_ptr<wrench::StorageService>> matched_storage_services;
    for (auto const &ss : this->storage_services) {
        if (ss->getHostname() == hostname) {
            matched_storage_services.push_back(ss);
        }
    }

    // TODO: right now, there are loopkupFile() calls, which simulate overhead. Could be replaced
    // TODO: by a lookup of the SimpleExecutionController::global_file_map data structure in case
    // TODO: simulating that overhead is not desired/necessary.Perhaps an option of the simulator?

    // For each file, identify where to read it from and/or deal with cache updates, etc.
    for (auto const &f : this->files) {
        // find a source providing the required file
        std::shared_ptr<wrench::StorageService> source_ss;
        // See whether the file is already available in a "local" storage service
        for (auto const &ss : matched_storage_services) {
            if (ss->lookupFile(f, wrench::FileLocation::LOCATION(ss))) {
                source_ss = ss;
                break;
            }
        }
        // If yes, we're done
        if (source_ss) {
            SimpleSimulator::global_file_map[source_ss].touchFile(f);
            this->file_sources[f] = wrench::FileLocation::LOCATION(source_ss);
            continue;
        }
        // If not, then we have to copy the file from some source to some local storage service
        // TODO: Find the optimal source, whatever that means (right now it's whichever one works first)
        for (auto const &ss : this->storage_services) {
            if (ss->lookupFile(f, wrench::FileLocation::LOCATION(ss))) {
                source_ss = ss;
                break;
            }
        }
        if (!source_ss) {
            throw std::runtime_error("StreamedComputation(): Couldn't find file " + f->getID() + " on any storage service!");
        } else {
            SimpleSimulator::global_file_map[source_ss].touchFile(f);
        }

        // TODO: Find the optimal destination, whatever that means (right now it's random, with a bad RNG!)
        // TODO: But then perhaps matched_storage_services.size() is always 1? (see QUESTION above)
        auto destination_ss = matched_storage_services.at(rand() % matched_storage_services.size());

        // TODO: Instead of doing this file copy right here, instead instantly create the
        // TODO: file instantly locally for next jobs? But then the second job
        // TODO: could complete before the first job, which doesn't
        // TODO: Perhaps make subsequent job wait for completion of the first
        // TODO: job so as not to finish earlier than that first job. Better idea
        // TODO: perhaps: have the first job that streams the file update a counter
        // TODO: of file blocks available at the storage service, and subsequent jobs
        // TODO: can read a block only if it's available (e.g., by waiting on some
        // TODO: condition variable, which is signaled by the first job each time it
        // TODO: reads a block).

        // Evict files while to create space, using an LRU scheme!
        double free_space = destination_ss->getFreeSpace().begin()->second;
        while (free_space < f->getSize()) {
            auto to_evict = SimpleSimulator::global_file_map[destination_ss].removeLRUFile();
            WRENCH_INFO("Evicting file %s from storage service on host %s",
                        to_evict->getID().c_str(), destination_ss->getHostname().c_str());
            destination_ss->deleteFile(to_evict, wrench::FileLocation::LOCATION(destination_ss));
            free_space += to_evict->getSize();
        }


        // Do the copy
        wrench::StorageService::copyFile(f, wrench::FileLocation::LOCATION(source_ss), wrench::FileLocation::LOCATION(destination_ss));
        SimpleSimulator::global_file_map[destination_ss].touchFile(f);


        this->file_sources[f] = wrench::FileLocation::LOCATION(destination_ss);
    }
}

//? put this into the other determine function to prevent two times the same loop?
double StreamedComputation::determineTotalDataSize(const std::vector<std::shared_ptr<wrench::DataFile>> &files) {
    double incr_file_size;
    for (auto const &f : this->files) {
        incr_file_size += f->getSize();
    }
    return incr_file_size;
}



void StreamedComputation::operator () (std::shared_ptr<wrench::ActionExecutor> action_executor) {
    std::string hostname = action_executor->getHostname();

    // Identify all file sources (and deal with caching, evictions, etc.
    WRENCH_INFO("Determining file sources for streamed computation");
    this->determineFileSources(hostname);

    this->performComputation(hostname);

}

double StreamedComputation::determineFlops(double data_size, double total_data_size) {
    double flops = this->total_flops * data_size / total_data_size;
    return flops;
}

void StreamedComputation::performComputation(std::string &hostname) {
    WRENCH_INFO("Performing streamed computation!");
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
            double num_flops = determineFlops(num_bytes, this->total_data_size);
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

        // Process last blocks
        double num_flops = determineFlops(std::min<double>(SimpleSimulator::xrd_block_size, data_to_process), this->total_data_size);
        simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(num_flops);
        exec->start();
        exec->wait();

    }

}

