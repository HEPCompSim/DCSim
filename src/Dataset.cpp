#include "Dataset.h"

/**
 * @brief
 *
 * @param num_jobs: number of tasks
 * @param infiles_per_task: number of input-files each job processes
 * @param average_flops: expectation value of the flops (truncated gaussian) distribution
 * @param sigma_flops: std. deviation of the flops (truncated gaussian) distribution
 * @param average_memory: expectation value of the memory (truncated gaussian) distribution
 * @param sigma_memory: std. deviation of the memory (truncated gaussian) distribution
 * @param average_infile_size: expectation value of the input-file size (truncated gaussian) distribution
 * @param sigma_infile_size: std. deviation of the input-file size (truncated gaussian) distribution
 * @param average_outfile_size: expectation value of the output-file size (truncated gaussian) distribution
 * @param sigma_outfile_size: std. deviation of the output-file size (truncated gaussian) distribution
 * @param workload_type: flag to specifiy, whether the job should run with streaming or not
 * @param name_suffix: part of job name to distinguish between different workloads
 * @param arrival_time: submission time offset relative to simulation start
 * @param generator: random number generator objects to draw from
 *
 * @throw std::runtime_error
 */
Dataset::Dataset(
    const std::vector<std::string> hostname,
    const double average_infile_size, const double sigma_infile_size,
    const std::string name_suffix,
    const std::mt19937 &generator)
{
    this->generator = generator;
}
