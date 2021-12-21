

#ifndef S_JOB_SPECIFICATION_H
#define S_JOB_SPECIFICATION_H

#include <wrench-dev.h>

struct JobSpecification {
public:
    bool streaming_enabled = false;
    bool simplified_streaming = false;
    std::vector<std::shared_ptr<wrench::DataFile>> infiles;
    double flops = 0.0;
    double mem = 0.0;
    std::shared_ptr<wrench::DataFile> outfile;

};
#endif //S_JOB_SPECIFICATION_H
