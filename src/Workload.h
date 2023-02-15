
#ifndef S_WORKLOAD_H
#define S_WORKLOAD_H

#include "JobSpecification.h"
#include "util/Utils.h"


#define WORKLOAD_TYPES( F ) \
    F(Calculation) \
    F(Streaming) \
    F(Copy)

#define F(type) type ,
enum WorkloadType { WORKLOAD_TYPES( F ) NumWorkloadTypes };
#undef F

std::string workload_type_to_string( WorkloadType );
// WorkloadType string_to_workload_type( const std::string& );


// enum class WorkloadType {Calculation, Streaming, Copy};

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
            const size_t num_jobs,
            const size_t infiles_per_task,
            const double average_flops, const double sigma_flops,
            const double average_memory, const double sigma_memory,
            const double average_infile_size, const double sigma_infile_size,
            const double average_outfile_size, const double sigma_outfile_size,
            const WorkloadType workload_type, const std::string name_suffix,
            const double arrival_time,
            const std::mt19937& generator
        );

        // job list with specifications
        std::vector<JobSpecification> job_batch;
        // Usage of block streaming
        WorkloadType workload_type;
        // time offset until job submission relative to simulation start time (0)
        double submit_arrival_time;

    private:
        /** @brief generator to shuffle jobs **/
        std::mt19937 generator;
};


#endif //S_WORKLOAD_H
