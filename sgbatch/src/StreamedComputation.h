

#ifndef S_STREAMEDCOMPUTATION_H
#define S_STREAMEDCOMPUTATION_H

#include <wrench-dev.h>

#include "SimpleSimulator.h"

class StreamedComputation {

public:
    // TODO: REMOVE MOST THINGS IN HERE AND RELY ON THE GLOBALS IN SimpleSimulation::...
    StreamedComputation(std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                        std::vector<std::shared_ptr<wrench::DataFile>> &files,
                        double total_flops);

    void determineFileSources(std::string hostname);

    void operator () (std::shared_ptr<wrench::ActionExecutor> action_executor);

    double determineFlops(double data_size, double total_data_size);

    void performComputation(std::string &hostname);

private:
    std::set<std::shared_ptr<wrench::StorageService>> storage_services;
    std::vector<std::shared_ptr<wrench::DataFile>> files; //? does this need to be ordered?
    double total_flops;

    std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>> file_sources;

    double determineTotalDataSize(const std::vector<std::shared_ptr<wrench::DataFile>> &files);
    double total_data_size;
};

#endif //S_STREAMEDCOMPUTATION_H
