

#ifndef S_STREAMEDCOMPUTATION_H
#define S_STREAMEDCOMPUTATION_H

#include <wrench-dev.h>

#include "SimpleSimulator.h"

class StreamedComputation {

public:
    // TODO: REMOVE MOST THINGS IN HERE AND RELY ON THE GLOBALS IN SimpleSimulation::...
    StreamedComputation(std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                        std::vector<std::shared_ptr<wrench::DataFile>> &files);

    void determineFileSources(std::string hostname);

    void operator () (std::shared_ptr<wrench::ActionExecutor> action_executor);

    static double determineFlops(double data_size);

    void performComputationNoStreaming(std::string &hostname);

    void performComputationStreaming(std::string &hostname);

private:
    std::set<std::shared_ptr<wrench::StorageService>> storage_services;
    std::vector<std::shared_ptr<wrench::DataFile>> files;

    std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>> file_sources;
};

#endif //S_STREAMEDCOMPUTATION_H
