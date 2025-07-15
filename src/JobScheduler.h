

#ifndef DCSIM_JOBSCHEDULER_H
#define DCSIM_JOBSCHEDULER_H

#include <wrench-dev.h>
#include <iostream>
#include <vector>

class WorkloadExecutionController;

class JobScheduler {

public:

    explicit JobScheduler(const std::vector<std::shared_ptr<wrench::ComputeService>> &compute_services);
    void addExecutionController(WorkloadExecutionController *execution_controller);
    void schedule();
    void jobDone(const std::shared_ptr<wrench::CompoundJob> &job);

private:
    std::map<std::shared_ptr<wrench::ComputeService>, std::tuple<unsigned long, sg_size_t>> available_resources;
    std::vector<WorkloadExecutionController *> execution_controllers;
    unsigned long total_num_idle_cores;

    std::shared_ptr<wrench::ComputeService> pickComputeService(unsigned long num_cores, sg_size_t total_ram);

};

#endif //DCSIM_JOBSCHEDULER_H
