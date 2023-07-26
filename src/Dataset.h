
#ifndef S_DATASET_H
#define S_DATASET_H

#include "JobSpecification.h"
#include "util/Utils.h"

class Dataset {
    public:
        // Constructor
        Dataset(
            const std::vector<std::string> hostname, const double num_files,
            nlohmann::json file_size,
            const std::string name_suffix,
            const std::mt19937& generator
        );
        std::vector<std::string> hostnames;
        std::vector<std::shared_ptr<wrench::DataFile>> files;
        std::string name;

    private:
        std::function<double(std::mt19937&)> size_dist;
        std::function<double(std::mt19937&)> initializeRNG(nlohmann::json json);
        std::mt19937 generator;
};


#endif //S_DATASET_H
