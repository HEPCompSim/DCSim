
#ifndef S_DATASET_H
#define S_DATASET_H

#include "JobSpecification.h"
#include "util/Utils.h"

class Dataset {
    public:
        // Constructor
        Dataset(
            const std::vector<std::string> hostname, const double num_files,
            const double average_file_size, const double sigma_file_size,
            const std::string name_suffix,
            const std::mt19937& generator
        );

        std::vector<std::shared_ptr<wrench::DataFile>> files;

    private:
        /** @brief generator to shuffle jobs **/
        std::mt19937 generator;
};


#endif //S_DATASET_H
