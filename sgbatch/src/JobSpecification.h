

#ifndef S_JOB_SPECIFICATION_H
#define S_JOB_SPECIFICATION_H

#include <wrench-dev.h>

struct JobSpecification {
public:
    std::vector<std::shared_ptr<wrench::DataFile>> infiles;
    std::shared_ptr<wrench::DataFile> outfile;
    std::shared_ptr<wrench::FileLocation> outfile_destination;
    double total_flops;
    double total_mem;

};
#endif //S_JOB_SPECIFICATION_H
