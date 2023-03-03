

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
        const double average_flops, const double sigma_flops,
        const double average_memory, const double sigma_memory,
        const double average_infile_size, const double sigma_infile_size,
        const double average_outfile_size, const double sigma_outfile_size,
        const enum WorkloadType workload_type, const std::string name_suffix,
        const double arrival_time,
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
    std::normal_distribution<> insize_dist(average_infile_size, sigma_infile_size);
    std::normal_distribution<> outsize_dist(average_outfile_size,sigma_outfile_size);

    for (size_t j = 0; j < num_jobs; j++) {

        // Create a job specification
        JobSpecification job_specification;

        // Sample strictly positive task flops
        double dflops = flops_dist(this->generator);
        while ((average_flops+sigma_flops) < dflops || dflops < 0.) dflops = flops_dist(this->generator);
        job_specification.total_flops = dflops;

        // Sample strictly positive task memory requirements
        double dmem = mem_dist(this->generator);
        while ((average_memory+sigma_memory) < dmem || dmem < 0.) dmem = mem_dist(this->generator);
        job_specification.total_mem = dmem;

        for (size_t f = 0; f < infiles_per_task; f++) {
            // Sample inputfile sizes
            double dinsize = insize_dist(this->generator);
            while ((average_infile_size+3*sigma_infile_size) < dinsize || dinsize < 0.) dinsize = insize_dist(this->generator);
            job_specification.infiles.push_back(wrench::Simulation::addFile("infile_" + name_suffix + potential_separator + std::to_string(j) + "_" + std::to_string(f), dinsize));
        }

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
}

/**
 * @brief Fill a Workload consisting of jobs with job specifications, 
 * which include the inputfile and outputfile dependencies.
 * It can be chosen between jobs streaming input data and perform computations simultaneously 
 * or jobs copying the full input-data and compute afterwards.
 *    
 * @param num_jobs: number of tasks
 * @param infiles_per_task: number of input-files each job processes
 * @param flops: json object containing type and parameters for the flops distribution
 * @param memory: json object containing type and parameters for the memory distribution
 * @param infile_size: json object containing type and parameters for the input-file size distribution
 * @param outfile_size: json object containing type and parameters for the output-file size distribution
 * @param workload_type: flag to specifiy, whether the job should run with streaming or not
 * @param name_suffix: part of job name to distinguish between different workloads
 * @param arrival_time: submission time offset relative to simulation start
 * @param generator: random number generator objects to draw from
 * 
 * @throw std::runtime_error
 */
Workload::Workload(
        const size_t num_jobs,
        const size_t infiles_per_job,
        nlohmann::json flops,
        nlohmann::json memory,
        nlohmann::json infile_size,
        nlohmann::json outfile_size,
        const enum WorkloadType workload_type, const std::string name_suffix,
        const double arrival_time,
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
    this->flops_dist = [&]() {
        if(flops["type"]=="gaussian") {
            return std::normal_distribution<double>(flops["average"],flops["sigma"])(this->generator);
        } else if(flops["type"]=="histogram") {
            return std::piecewise_constant_distribution<double>(flops["bins"].begin(),flops["bins"].end(),flops["counts"].begin())(this->generator);
        } else {
            throw std::runtime_error("Random number generation for type " + std::string(flops["type"]) + " not implemented!");
        }
    };
    this->mem_dist = Workload::initializeRNG(memory);
    this->insize_dist = Workload::initializeRNG(infile_size);
    this->outsize_dist = Workload::initializeRNG(outfile_size);

    for (size_t j = 0; j < num_jobs; j++) {
        batch.push_back(sampleJob(j, infiles_per_job, name_suffix, potential_separator));
    }

    this->job_batch = batch;
    this->workload_type = workload_type;
    this->submit_arrival_time = arrival_time;
}


std::function<double()> Workload::initializeRNG(nlohmann::json json) {
    std::function<double()> dist = [&]() {
        if(json["type"]=="gaussian") {
            return std::normal_distribution<double>(json["average"],json["sigma"])(this->generator);
        } else if(json["type"]=="histogram") {
            return std::piecewise_constant_distribution<double>(json["bins"].begin(),json["bins"].end(),json["counts"].begin())(this->generator);
        } else {
            throw std::runtime_error("Random number generation for type " + std::string(json["type"]) + " not implemented!");
        }
    };
    return dist;
}


JobSpecification Workload::sampleJob(size_t job_id, const size_t infiles_per_job, std::string name_suffix, std::string potential_separator) {
    // Create a job specification
    JobSpecification job_specification;

    size_t j = job_id;

    // Sample strictly positive task flops
    double dflops = this->flops_dist();
    while (dflops < 0.) dflops = this->flops_dist();
    job_specification.total_flops = dflops;

    // Sample strictly positive task memory requirements
    double dmem = this->mem_dist();
    while (dmem < 0.) dmem = this->mem_dist();
    job_specification.total_mem = dmem;

    for (size_t f = 0; f < infiles_per_job; f++) {
        // Sample strictly positive inputfile sizes
        double dinsize = this->insize_dist();
        while (dinsize < 0.) dinsize = this->insize_dist();
        job_specification.infiles.push_back(wrench::Simulation::addFile("infile_" + name_suffix + potential_separator + std::to_string(j) + "_" + std::to_string(f), dinsize));
    }

    // Sample outfile sizes
    double doutsize = this->outsize_dist();
    while (doutsize < 0.) doutsize = this->outsize_dist();
    job_specification.outfile = wrench::Simulation::addFile("outfile_" + name_suffix + potential_separator + std::to_string(j), doutsize);

    job_specification.jobid = "job_" + name_suffix + potential_separator + std::to_string(j);

    return job_specification;
}
