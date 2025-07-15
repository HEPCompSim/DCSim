#include "Workload.h"
#include "Dataset.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(workload, "Log category for WorkloadExecutionController");


#define F(type) #type,
const char *workload_type_names[] = {WORKLOAD_TYPES(F) nullptr};
#undef F

std::string workload_type_to_string(WorkloadType workload) {
    return ((int) workload < (int) NumWorkloadTypes)
                   ? workload_type_names[(int) workload]
                   : "";
}

// WorkloadType string_to_workload_type( const std::string& s )
// {
//   return std::find( workload_type_names, workload_type_names + NumWorkloadTypes, s ) - workload_type_names;
//   // You could raise an exception if the above expression == NumTokenTypes, if you wanted
// }


/**
 * @brief Fill a Workload consisting of jobs with job specifications, 
 * which include the input file and outputfile dependencies.
 * One can chose between jobs streaming input data and perform computations simultaneously
 * or jobs copying the full input-data and performing the computation afterward.
 *    
 * @param num_jobs: number of tasks
 * @param cores: json object containing type and parameters for the cores distribution
 * @param flops: json object containing type and parameters for the flops distribution
 * @param memory: json object containing type and parameters for the memory distribution
 * @param outfile_size: json object containing type and parameters for the output-file size distribution
 * @param name_suffix: part of job name to distinguish between different workloads
 * @param arrival_time: submission time offset relative to simulation start
 * @param generator: random number generator objects to draw from
 * @param infile_datasets the input file dataset
 * 
 * @throw std::runtime_error
 */
Workload::Workload(
        const size_t num_jobs,
        nlohmann::json cores,
        nlohmann::json flops,
        nlohmann::json memory,
        nlohmann::json outfile_size,
        const enum WorkloadType workload_type,
        const std::string &name_suffix,
        const double arrival_time,
        const std::mt19937 &generator,
        const std::vector<std::string> &infile_datasets) {
    this->generator = generator;
    // Map to store the workload specification
    std::vector<JobSpecification> batch;
    std::string potential_separator = (name_suffix.empty() ? "" : "_");

    // Initialize random number generators
    this->core_dist = Workload::createIntRNG(cores);
    this->flops_dist = Workload::createDoubleRNG(flops);
    this->mem_dist = Workload::createDoubleRNG(memory);
    this->outsize_dist = Workload::createDoubleRNG(outfile_size);
    for (size_t j = 0; j < num_jobs; j++) {
        batch.push_back(sampleJob(j, name_suffix, potential_separator));
    }

    this->job_batch = batch;
    this->workload_type = workload_type;
    this->submit_arrival_time = arrival_time;
    if (!infile_datasets.empty())
        this->infile_datasets = infile_datasets;
}

std::function<double(std::mt19937 &)> Workload::createDoubleRNG(nlohmann::json json) {
    // std::cerr << json["type"] << ": ";
    std::function<double(std::mt19937 &)> dist;
    if (json["type"].get<std::string>() == "gaussian") {
        double ave = json["average"].get<double>();
        double sigma = json["sigma"].get<double>();
        // std::cerr << "ave: "<< ave << ", stddev: " << sigma << std::endl;
        dist = [ave, sigma](std::mt19937 &generator) {
            return std::normal_distribution<double>(ave, sigma)(generator);
        };
    } else if (json["type"].get<std::string>() == "histogram") {
        auto bins = json["bins"].get<std::vector<double>>();
        auto weights = json["counts"].get<std::vector<int>>();
        // std::cerr << "bins: " << json["bins"] << ", weights: " << json["counts"] << std::endl;
        dist = [bins, weights](std::mt19937 &generator) {
            return std::piecewise_constant_distribution<double>(bins.begin(), bins.end(), weights.begin())(generator);
        };
    } else {
        throw std::runtime_error("Random number generation for type " + json["type"].get<std::string>() + " not implemented for real valued distributions!");
    }
    return dist;
}

std::function<int(std::mt19937 &)> Workload::createIntRNG(nlohmann::json json) {
    // std::cerr << json["type"] << ": ";
    std::function<int(std::mt19937 &)> dist;
    if (json["type"].get<std::string>() == "poisson") {
        int mu = json["mu"].get<int>();
        // std::cerr << "ave: "<< ave << ", stddev: " << sigma << std::endl;
        dist = [mu](std::mt19937 &generator) {
            return std::poisson_distribution<int>(mu)(generator);
        };
    } else if (json["type"].get<std::string>() == "histogram") {
        try {
            auto bins = json["bins"].get<std::vector<double>>();
            WRENCH_WARN("Ignoring configured bins for integer distribution!");
        } catch (...) {}
        auto weights = json["counts"].get<std::vector<int>>();
        // std::cerr << "bins: " << json["bins"] << ", weights: " << json["counts"] << std::endl;
        dist = [weights](std::mt19937 &generator) {
            return std::discrete_distribution<int>(weights.begin(), weights.end())(generator);
        };
    } else {
        throw std::runtime_error("Random number generation for type " + json["type"].get<std::string>() + " not implemented for integer valued distributions!");
    }
    return dist;
}


JobSpecification Workload::sampleJob(size_t job_id, const std::string &name_suffix, const std::string &potential_separator) {
    // Create a job specification
    JobSpecification job_specification;

    size_t j = job_id;

    // Sample number of cores to run on
    int req_cores = this->core_dist(this->generator);
    while (req_cores < 1) req_cores = this->core_dist(this->generator);
    job_specification.cores = req_cores;

    // Sample strictly positive task flops
    double dflops = this->flops_dist(this->generator);
    while (dflops < 0.) dflops = this->flops_dist(this->generator);
    job_specification.total_flops = dflops;

    // Sample strictly positive task memory requirements
    double dmem = this->mem_dist(this->generator);
    while (dmem < 0.) dmem = this->mem_dist(this->generator);
    auto dmem_in_bytes = static_cast<sg_size_t>(dmem);
    job_specification.total_mem = dmem_in_bytes;

    // Sample outfile sizes
    double doutsize = this->outsize_dist(this->generator);
    while (doutsize < 0.) doutsize = this->outsize_dist(this->generator);
    auto doutsize_in_bytes = static_cast<sg_size_t>(doutsize);
    job_specification.outfile = wrench::Simulation::addFile("outfile_" + name_suffix + potential_separator + std::to_string(j), doutsize_in_bytes);

    job_specification.jobid = "job_" + name_suffix + potential_separator + std::to_string(j);

    return job_specification;
}

template<class InputIt, class OutputIt, class Pred, class Fct>
void transform_if(InputIt first, InputIt last, OutputIt dest, Pred pred, Fct transform) {
    while (first != last) {
        if (pred(*first))
            *dest++ = transform(*first);
        ++first;
    }
}

void Workload::assignFiles(std::vector<Dataset> const &dataset_specs) {
    std::vector<Dataset const *> matching_ds{};
    transform_if(
            dataset_specs.begin(), dataset_specs.end(), std::back_inserter(matching_ds), [&](Dataset const &ds) { return std::find(infile_datasets.begin(), infile_datasets.end(), ds.name) != infile_datasets.end(); },
            [&](Dataset const &ds) { return &ds; });
    if (matching_ds.empty())
        throw std::runtime_error("ERROR: no valid infile dataset name in workload configuration.");
    size_t num_files = std::accumulate(matching_ds.begin(), matching_ds.end(), 0, [](int sum, Dataset const *ds) { return sum + ds->files.size(); });
    std::vector<std::shared_ptr<wrench::DataFile>> all_files{};
    all_files.reserve(num_files);
    for (auto const &ds: matching_ds) {
        std::copy(ds->files.begin(), ds->files.end(), std::back_inserter(all_files));
    }
    size_t num_jobs = job_batch.size();
    if (num_jobs == 0)
        return;
    // int num_files = all_files.size();
    size_t k = num_files / num_jobs;
    std::cerr << "Assigning " << num_files << " files to " << num_jobs << " jobs\n";
    for (auto j = 0; j < num_jobs; ++j) {
        auto beg_it = all_files.begin() + j * k;
        if (std::distance(beg_it, all_files.end()) < k) {
            std::copy(beg_it, all_files.end(), std::back_inserter(job_batch[j].infiles));
            break;
        }
        std::copy_n(beg_it, k, std::back_inserter(job_batch[j].infiles));
    }
}
