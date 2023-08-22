

#ifndef S_JOB_SPECIFICATION_H
#define S_JOB_SPECIFICATION_H

#include <wrench-dev.h>

#include "util/Utils.h"

/**
 * @brief Container to hold all job specific information
 * 
 */
struct JobSpecification {
public:
    // identifier
    std::string jobid;
    // Input files to process
    std::vector<std::shared_ptr<wrench::DataFile>> infiles;
    // Output file to write by the job
    std::shared_ptr<wrench::DataFile> outfile;
    // Desired destination of the output file to be written to
    std::shared_ptr<wrench::FileLocation> outfile_destination;
    // Number of cores to run on
    int cores;
    // Total number of FLOPS to be computed for the job to finish
    double total_flops;
    // Memory consumption of the job
    double total_mem;
};

#endif//S_JOB_SPECIFICATION_H
