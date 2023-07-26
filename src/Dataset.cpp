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
    nlohmann::json file_size,
    const std::string name_suffix,
    const std::mt19937 &generator)
{
    this->generator = generator;
        std::string potential_separator = "_";
    if(name_suffix == ""){
        potential_separator = "";
    }

    this->size_dist = initializeRNG(file_size);
    for (size_t f = 0; f < num_files; f++) {
        // Sample inputfile sizes
        double dsize = size_dist(this->generator);
        while (dsize < 0.) dsize = this->size_dist(this->generator);
        files.push_back(wrench::Simulation::addFile("infile_" + name_suffix + potential_separator + std::to_string(f), dsize));
    }
    this->hostnames = hostnames;
    this->name = name_suffix;
}

std::function<double(std::mt19937&)> Dataset::initializeRNG(nlohmann::json json) {
    std::cerr << json["type"] << ": ";
    std::function<double(std::mt19937&)> dist;
    if(json["type"].get<std::string>()=="gaussian") {
        double ave = json["average"].get<double>();
        double sigma = json["sigma"].get<double>();
        std::cerr << "ave: "<< ave << ", stddev: " << sigma << std::endl;
        dist = [ave, sigma](std::mt19937& generator){
            return std::normal_distribution<double>(ave, sigma)(generator);
        };
    } else if(json["type"].get<std::string>()=="histogram") {
        auto bins = json["bins"].get<std::vector<double>>();
        auto weights = json["counts"].get<std::vector<int>>();
        std::cerr << "bins: " << json["bins"] << ", weights: " << json["counts"] << std::endl;
        dist = [bins, weights](std::mt19937& generator){
            return std::piecewise_constant_distribution<double>(bins.begin(),bins.end(),weights.begin())(generator);
        };
    } else {
        throw std::runtime_error("Random number generation for type " + json["type"].get<std::string>() + " not implemented!");
    }
    return dist;
}