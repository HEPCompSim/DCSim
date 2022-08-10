

#ifndef S_JOB_SPECIFICATION_H
#define S_JOB_SPECIFICATION_H

#include <wrench-dev.h>

/**
 * @brief Container to hold all job specific information
 * 
 */
struct JobSpecification {
public:
    // Input files to process
    std::vector<std::shared_ptr<wrench::DataFile>> infiles;
    // Output file to write by the job
    std::shared_ptr<wrench::DataFile> outfile;
    // Desired destination of the output file to be written to
    std::shared_ptr<wrench::FileLocation> outfile_destination;
    // Total number of FLOPS to be computed for the job to finish
    double total_flops;
    // Memory consumption of the job
    double total_mem;
    // Usage of block streaming
    bool use_blockstreaming;

};
#endif //S_JOB_SPECIFICATION_H
