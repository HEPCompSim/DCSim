
#ifndef S_WORKLOAD_H
#define S_WORKLOAD_H

#include "JobSpecification.h"
#include "util/Utils.h"
#include "Dataset.h"

// #include <variant>

#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>


#define WORKLOAD_TYPES(F) \
    F(Calculation)        \
    F(Streaming)          \
    F(Copy)

#define F(type) type,
enum WorkloadType { WORKLOAD_TYPES(F) NumWorkloadTypes };
#undef F

std::string workload_type_to_string(WorkloadType);
// WorkloadType string_to_workload_type( const std::string& );


// enum class WorkloadType {Calculation, Streaming, Copy};

inline WorkloadType get_workload_type(const std::string &wfname) {
    if (wfname == "calculation") {
        return WorkloadType::Calculation;
    } else if (wfname == "streaming") {
        return WorkloadType::Streaming;
    } else if (wfname == "copy") {
        return WorkloadType::Copy;
    } else {
        throw std::runtime_error("Workload type " + wfname + " invalid. Please choose 'calculation', 'streaming', or 'copy'");
    }
}

// std::string workloadtype_to_string(WorkloadType type) {
//     if (type == WorkloadType::Calculation) return "Calculation";
//     else if (type == WorkloadType::Streaming) return "Streaming";
//     else if (type == WorkloadType::Copy) return "Copy";
//     else throw std::runtime_error("Couldn't cast WorkflowType to string!");
// }


class Workload {
public:
    // Constructor
    Workload(
            size_t num_jobs,
            nlohmann::json cores,
            nlohmann::json flops,
            nlohmann::json memory,
            nlohmann::json outfile_size,
            WorkloadType workload_type, const std::string &name_suffix,
            double arrival_time, const std::mt19937 &generator,
            const std::vector<std::string> &infile_datasets = {});

    // job list with specifications
    std::vector<JobSpecification> job_batch;
    // Usage of block streaming
    WorkloadType workload_type;
    // time offset until job submission relative to simulation start time (0)
    double submit_arrival_time;
    // infile dataset name
    std::vector<std::string> infile_datasets;
    void assignFiles(std::vector<Dataset> const &);

private:
    /** @brief generator to shuffle jobs **/
    std::mt19937 generator;
    std::function<int(std::mt19937 &)> core_dist;
    std::function<double(std::mt19937 &)> flops_dist;
    std::function<double(std::mt19937 &)> mem_dist;
    std::function<double(std::mt19937 &)> outsize_dist;

    static std::function<int(std::mt19937 &)> createIntRNG(nlohmann::json json);
    static std::function<double(std::mt19937 &)> createDoubleRNG(nlohmann::json json);

    JobSpecification sampleJob(size_t job_id, const std::string &name_suffix, const std::string &potential_separator);
};


#endif//S_WORKLOAD_H
