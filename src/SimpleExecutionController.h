/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef MY_SIMPLE_EXECUTION_CONTROLLER_H
#define MY_SIMPLE_EXECUTION_CONTROLLER_H

#include <wrench-dev.h>
#include <iostream>
#include <fstream>

#include "JobSpecification.h"
#include "LRU_FileList.h"

#include "util/Enums.h"

class Simulation;

class SimpleExecutionController : public wrench::ExecutionController {
public:
    // Constructor
    SimpleExecutionController(
              const std::map<std::string, JobSpecification> &workload_spec,
              const std::set<std::shared_ptr<wrench::HTCondorComputeService>>& htcondor_compute_services,
              const std::set<std::shared_ptr<wrench::StorageService>>& grid_storage_services,
              const std::set<std::shared_ptr<wrench::StorageService>>& cache_storage_services,
              //const std::set<std::shared_ptr<wrench::NetworkProximityService>>& network_proximity_services,
              //std::shared_ptr<wrench::FileRegistryService> file_registry_service,
              const std::string& hostname,
              //const double& hitrate,
              const std::string& outputdump_name);

    std::map<std::string, JobSpecification>& get_workload_spec() {
        return this->workload_spec;
    }

    void set_workload_spec(std::map<std::string, JobSpecification> w) {
        this->workload_spec = w;
    }


protected:
    void processEventCompoundJobFailure(std::shared_ptr<wrench::CompoundJobFailedEvent>) override;
    void processEventCompoundJobCompletion(std::shared_ptr<wrench::CompoundJobCompletedEvent>) override;

private:

    std::set<std::shared_ptr<wrench::HTCondorComputeService>> htcondor_compute_services;
    std::set<std::shared_ptr<wrench::StorageService>> grid_storage_services;
    std::set<std::shared_ptr<wrench::StorageService>> cache_storage_services;
    std::map<std::string, JobSpecification> workload_spec;


    int main() override;

    /** @brief The job manager */
    std::shared_ptr<wrench::JobManager> job_manager;
    /** @brief The data movement manager */
    std::shared_ptr<wrench::DataMovementManager> data_movement_manager;
    /** @brief Whether the workflow execution shoulb be aborted */
    bool abort = false;
    /** @brief The desired fraction of input files served by the cache */
    double hitrate = 0.;

    /** @brief Map holding information about the first and last task of jobs for output dump */
//    std::map<std::shared_ptr<wrench::StandardJob>, std::pair<wrench::WorkflowTask*, wrench::WorkflowTask*>> job_first_last_tasks;
    /** @brief Filename for the output-dump file */
    std::string filename;
    /** @brief Output filestream object to write out dump */
    std::ofstream filedump;

    /** @brief number of complete jobs so far **/
    size_t num_completed_jobs = 0;

};

#endif //MY_SIMPLE_EXECUTION_CONTROLLER_H

