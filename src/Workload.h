
#ifndef S_WORKLOAD_H
#define S_WORKLOAD_H

#include "JobSpecification.h"
#include "util/Utils.h"

class Workload {
    public:
        // Constructor
        Workload(
            const size_t num_jobs,
            const size_t infiles_per_task,
            const double average_flops, const double sigma_flops,
            const double average_memory, const double sigma_memory,
            const double average_infile_size, const double sigma_infile_size,
            const double average_outfile_size, const double sigma_outfile_size,
            const WorkloadType workload_type, const std::string name_suffix,
            const double time_offset,
            const std::mt19937& generator
        );

        // job list with specifications
        std::vector<JobSpecification> job_batch;
        // Usage of block streaming
        WorkloadType workload_type;
        // time offset until job submission relative to simulation start time (0)
        double submit_time_offset;
};


enum WorkloadType {Calculation, Streaming, Copy};

inline WorkloadType get_workload_type(std::string wfname) {
    if(wfname == "calculation") {
        return WorkloadType::Calculation;
    }
    else if (wfname == "streaming") {
        return WorkloadType::Streaming;
    }
    else if (wfname == "copy") {
        return WorkloadType::Copy;
    }
    else {
        throw std::runtime_error("Workload type " + wfname + " invalid. Please choose 'calculation', 'streaming', or 'copy'");
    }
}


#endif //S_WORKLOAD_H
