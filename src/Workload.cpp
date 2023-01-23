

#include "Workload.h"


/**
 * @brief Fill a Workload consisting of jobs with job specifications, 
 * which include the inputfile and outputfile dependencies.
 * It can be chosen between jobs streaming input data and perform computations simultaneously 
 * or jobs copying the full input-data and compute afterwards.
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
 * @param time_offset: submission time offset relative to simulation start
 * @param generator: random number generator objects to draw from
 * 
 * @throw std::runtime_error
 */
Workload::Workload(
        const size_t num_jobs,
        const size_t infiles_per_task,
        const double average_flops, const double sigma_flops,
        const double average_memory, const double sigma_memory,
        const double average_infile_size, const double sigma_infile_size,
        const double average_outfile_size, const double sigma_outfile_size,
        const WorkloadType workload_type, const std::string name_suffix,
        const double time_offset,
        const std::mt19937& generator
) {
    // Map to store the workload specification
    std::vector<JobSpecification> batch;
    std::string potential_separator = "_";
    if(name_suffix == ""){
        potential_separator = "";
    }

    // Initialize random number generators
    std::normal_distribution<> flops_dist(average_flops, sigma_flops);
    std::normal_distribution<> mem_dist(average_memory, sigma_memory);
    std::normal_distribution<> insize_dist(average_infile_size, sigma_infile_size);
    std::normal_distribution<> outsize_dist(average_outfile_size,sigma_outfile_size);

    for (size_t j = 0; j < num_jobs; j++) {

        // Create a job specification
        JobSpecification job_specification;

        // Sample strictly positive task flops
        double dflops = flops_dist(generator);
        while ((average_flops+sigma_flops) < dflops || dflops < 0.) dflops = flops_dist(generator);
        job_specification.total_flops = dflops;

        // Sample strictly positive task memory requirements
        double dmem = mem_dist(generator);
        while ((average_memory+sigma_memory) < dmem || dmem < 0.) dmem = mem_dist(generator);
        job_specification.total_mem = dmem;

        for (size_t f = 0; f < infiles_per_task; f++) {
            // Sample inputfile sizes
            double dinsize = insize_dist(generator);
            while ((average_infile_size+3*sigma_infile_size) < dinsize || dinsize < 0.) dinsize = insize_dist(generator);
            job_specification.infiles.push_back(wrench::Simulation::addFile("infile_" + name_suffix + potential_separator + std::to_string(j) + "_" + std::to_string(f), dinsize));
        }

        // Sample outfile sizes
        double doutsize = outsize_dist(generator);
        while ((average_outfile_size+3*sigma_outfile_size) < doutsize || doutsize < 0.) doutsize = outsize_dist(generator);
        job_specification.outfile = wrench::Simulation::addFile("outfile_" + name_suffix + potential_separator + std::to_string(j), doutsize);

        job_specification.jobid = "job_" + name_suffix + potential_separator + std::to_string(j);
    }
    this->job_batch = batch;
    this->workload_type = workload_type;
    this->submit_time_offset = time_offset;
}
