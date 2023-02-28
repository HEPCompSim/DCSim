

#include "Workload.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(workload, "Log category for WorkloadExecutionController");


#define F(type) #type ,
const char* workload_type_names[] = { WORKLOAD_TYPES( F ) nullptr };
#undef F

std::string workload_type_to_string( WorkloadType workload )
{
  return ((int)workload < (int)NumWorkloadTypes)
    ? workload_type_names[ (int)workload ]
    : "";
}

// WorkloadType string_to_workload_type( const std::string& s )
// {
//   return std::find( workload_type_names, workload_type_names + NumWorkloadTypes, s ) - workload_type_names;
//   // You could raise an exception if the above expression == NumTokenTypes, if you wanted
// }


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
 * @param arrival_time: submission time offset relative to simulation start
 * @param generator: random number generator objects to draw from
 * 
 * @throw std::runtime_error
 */
Workload::Workload(
        const size_t num_jobs,
        const size_t infiles_per_task,
        const int request_cores,
        const double average_flops, const double sigma_flops,
        const double average_memory, const double sigma_memory,
        const double average_outfile_size, const double sigma_outfile_size,
        const enum WorkloadType workload_type, const std::string name_suffix,
        const std::string infile_dataset, const double arrival_time,
        const std::mt19937& generator
) {
    this->generator = generator;
    // Map to store the workload specification
    std::vector<JobSpecification> batch;
    std::string potential_separator = "_";
    if(name_suffix == ""){
        potential_separator = "";
    }

    // Initialize random number generators
    std::normal_distribution<> flops_dist(average_flops, sigma_flops);
    std::normal_distribution<> mem_dist(average_memory, sigma_memory);
    std::normal_distribution<> outsize_dist(average_outfile_size,sigma_outfile_size);

    for (size_t j = 0; j < num_jobs; j++) {

        // Create a job specification
        JobSpecification job_specification;

        // Set number of requested cores
        job_specification.cores = request_cores;

        // Sample strictly positive task flops
        double dflops = flops_dist(this->generator);
        while ((average_flops+sigma_flops) < dflops || dflops < 0.) dflops = flops_dist(this->generator);
        job_specification.total_flops = dflops;

        // Sample strictly positive task memory requirements
        double dmem = mem_dist(this->generator);
        while ((average_memory+sigma_memory) < dmem || dmem < 0.) dmem = mem_dist(this->generator);
        job_specification.total_mem = dmem;

        // Sample outfile sizes
        double doutsize = outsize_dist(this->generator);
        while ((average_outfile_size+3*sigma_outfile_size) < doutsize || doutsize < 0.) doutsize = outsize_dist(this->generator);
        job_specification.outfile = wrench::Simulation::addFile("outfile_" + name_suffix + potential_separator + std::to_string(j), doutsize);

        job_specification.jobid = "job_" + name_suffix + potential_separator + std::to_string(j);

        batch.push_back(job_specification);
    }
    this->job_batch = batch;
    this->workload_type = workload_type;
    this->submit_arrival_time = arrival_time;
    this->infile_dataset = infile_dataset;
}
