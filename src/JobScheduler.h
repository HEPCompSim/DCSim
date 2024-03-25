

#ifndef DCSIM_JOBSCHEDULER_H
#define DCSIM_JOBSCHEDULER_H

#include <wrench-dev.h>
#include <iostream>
#include <vector>

class WorkloadExecutionController;

class JobScheduler {

public:


    JobScheduler(const std::vector<std::shared_ptr<wrench::ComputeService>> &compute_services);
    void addExecutionController(WorkloadExecutionController *execution_controller);
    void schedule();
    void jobDone(const std::shared_ptr<wrench::CompoundJob> &job);
    
private:
    std::map<std::shared_ptr<wrench::ComputeService>, std::tuple<unsigned long, double>> available_resources;
    std::vector<std::shared_ptr<wrench::ComputeService>> compute_services;
    std::vector<WorkloadExecutionController *> execution_controllers;
    unsigned long total_num_idle_cores;

};


#endif //DCSIM_JOBSCHEDULER_H
