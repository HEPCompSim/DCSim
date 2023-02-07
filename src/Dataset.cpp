#include "Dataset.h"

/**
 * @brief
 *
 * @param hostnames: list of hostname on which the dataset is hosted
 * @param num_files: number of tasks
 * @param average_file_size: expectation value of the file size (truncated gaussian) distribution
 * @param sigma_file_size: std. deviation of the file size (truncated gaussian) distribution
 * @param name_suffix: dataset name
 * @param generator: random number generator objects to draw from
 *
 * @throw std::runtime_error
 */
Dataset::Dataset(
    const std::vector<std::string> hostnames, const double num_files,
    const double average_file_size, const double sigma_file_size,
    const std::string name_suffix,
    const std::mt19937 &generator)
{
    this->generator = generator;
        std::string potential_separator = "_";
    if(name_suffix == ""){
        potential_separator = "";
    }

    std::normal_distribution<> size_dist(average_file_size, sigma_file_size);
    for (size_t f = 0; f < num_files; f++) {
        // Sample inputfile sizes
        double dsize = size_dist(this->generator);
        while ((average_file_size+3*sigma_file_size) < dsize || dsize < 0.)
            dsize = size_dist(this->generator);
        files.push_back(wrench::Simulation::addFile("file_" + name_suffix + potential_separator + std::to_string(f), dsize));
    }    
}
