

#ifndef S_STREAMEDCOMPUTATION_H
#define S_STREAMEDCOMPUTATION_H

#include <wrench-dev.h>

class StreamedComputation {

public:
    StreamedComputation(std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                    std::vector<std::shared_ptr<wrench::DataFile>> &files,
                    double flops,
                    double mem) {
        this->storage_services = storage_services;
        this->files = files;
        this->flops = flops;
        this->mem = mem;
    }

    void operator () (std::shared_ptr<wrench::ActionExecutor> action_executor) {
        std::string hostname = action_executor->getHostname();

        // Identify all storage services that run on this host
        std::vector<std::shared_ptr<wrench::StorageService>> matched_storage_services;
        for (auto const &ss : this->storage_services) {
            if (ss->getHostname() == hostname) {
                matched_storage_services.push_back(ss);
            }
        }

        // TODO: RIGHT NOW, THERE ARE loopkupFile() calls, which simulate overhead. Could be replaced
        // TODO: by a lookup of the SimpleExecutionController::global_file_map data structure

        // For each file, identify where to read it from and/or deal with cache updates, etc.
        std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>> file_sources;
        for (auto const &f : this->files) {
            // See whether the file is already available in a "local" storage service
            std::shared_ptr<wrench::StorageService> source_ss;
            for (auto const &ss : matched_storage_services) {
                if (ss->lookupFile(f, wrench::FileLocation::LOCATION(ss))) {
                    source_ss = ss;
                    break;
                }
            }
            // If yes, we're done
            if (source_ss) {
                file_sources[f] = wrench::FileLocation::LOCATION(source_ss);
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
            }

            // TODO: Find the optimal destination, whatever that means (right now it's random, with a bad RNG!)
            auto destination_ss = matched_storage_services.at(rand() % matched_storage_services.size());

            // Evict files while to create space
            // TODO: Don't evict files at random but using some good scheme (which will require a more sophisticated data structure)
            double free_space = destination_ss->getFreeSpace().begin()->second;
            while (free_space < f->getSize()) {
                auto to_evict_it = SimpleExecutionController::global_file_map[destination_ss].begin();
                std::advance(to_evict_it, rand() % SimpleExecutionController::global_file_map[destination_ss].size());
                auto to_evict = *to_evict_it;
                destination_ss->deleteFile(to_evict, wrench::FileLocation::LOCATION(destination_ss));
                SimpleExecutionController::global_file_map[destination_ss].erase(f);
                free_space += to_evict->getSize();
            }

            // Do the copy
            wrench::StorageService::copyFile(f, wrench::FileLocation::LOCATION(source_ss), wrench::FileLocation::LOCATION(destination_ss));
            SimpleExecutionController::global_file_map[destination_ss].insert(f);

            file_sources[f] = wrench::FileLocation::LOCATION(destination_ss);

        }

        // At this point, we know where all files should be read from, we just need to implement the streaming
        // TODO: IMPLEMENT THE STREAMING
        // TODO: QUESTION: What is the chunk size for each file (the same?) and for the computation?


    }

private:
    std::set<std::shared_ptr<wrench::StorageService>> storage_services;
    std::vector<std::shared_ptr<wrench::DataFile>> files;
    double flops;
    double mem;

};

#endif //S_STREAMEDCOMPUTATION_H
