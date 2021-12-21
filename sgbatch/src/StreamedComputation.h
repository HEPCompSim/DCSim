

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
        std::set<std::shared_ptr<wrench::StorageService>> matched_storage_services;
        for (auto const &ss : this->storage_services) {
            if (ss->getHostname() == hostname) {
                matched_storage_services.insert(ss);
            }
        }

        // For each file, either read it from a matched service or copy it to a matched service,
        // dealing with eviction if needed
        for (auto const &f : this->files) {
            std::shared_ptr<wrench::StorageService> target_ss;
            for (auto const &ss : matched_storage_services) {
                if (ss->lookupFile(f, wrench::FileLocation::LOCATION(ss))) {
                    target_ss = ss;
                    break;
                }
            }
            if (target_ss) {

            }

        }


    }

private:
    std::set<std::shared_ptr<wrench::StorageService>> storage_services;
    std::vector<std::shared_ptr<wrench::DataFile>> files;
    double flops;
    double mem;

};

#endif //S_STREAMEDCOMPUTATION_H
