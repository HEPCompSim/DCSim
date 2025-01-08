/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef DCSIM_WORKLOAD_EXECUTION_CONTROLLER_H
#define DCSIM_WORKLOAD_EXECUTION_CONTROLLER_H

#include <wrench-dev.h>
#include <iostream>
#include <fstream>
#include <utility>

#include "JobSpecification.h"
#include "JobScheduler.h"
#include "Workload.h"
#include "LRU_FileList.h"

#include "util/Utils.h"

class Simulation;

class WorkloadExecutionController : public wrench::ExecutionController {
public:
    // Constructor
    WorkloadExecutionController(
            const Workload &workload_spec,
            const std::shared_ptr<JobScheduler> &job_scheduler,
            const std::set<std::shared_ptr<wrench::StorageService>> &grid_storage_services,
            const std::set<std::shared_ptr<wrench::StorageService>> &cache_storage_services,
            const std::string &hostname,
            const std::string &outputdump_name,
            const bool &shuffle_jobs, const std::mt19937 &generator);

    std::map<std::string, JobSpecification> &get_workload_spec() {
        return this->workload_spec;
    }

    void set_workload_spec(std::map<std::string, JobSpecification> w) {
        this->workload_spec = std::move(w);
    }

    std::shared_ptr<wrench::CompoundJob> createAndSubmitJob(const std::string &job_name,
                                                            const std::shared_ptr<wrench::ComputeService> &cs);
    void setJobSubmitted(const std::string &job_name);

    bool isWorkloadEmpty();

protected:
    void processEventCompoundJobFailure(std::shared_ptr<wrench::CompoundJobFailedEvent>);
    void processEventCompoundJobCompletion(std::shared_ptr<wrench::CompoundJobCompletedEvent>);

private:
    std::map<std::string, JobSpecification> workload_spec;
    std::map<std::string, JobSpecification> workload_spec_submitted;
    std::shared_ptr<JobScheduler> job_scheduler;

    std::set<std::shared_ptr<wrench::StorageService>> grid_storage_services;
    std::set<std::shared_ptr<wrench::StorageService>> cache_storage_services;

    int main() override;

    /** @brief The job manager */
    std::shared_ptr<wrench::JobManager> job_manager;
    // /** @brief The data movement manager */
    // std::shared_ptr<wrench::DataMovementManager> data_movement_manager;

    /** @brief Whether the workflow execution should be aborted */
    bool abort = false;
    /** @brief The desired fraction of input files served by the cache */
    double hitrate = 0.;

    /** @brief Map holding information about the first and last task of jobs for output dump */
    //    std::map<std::shared_ptr<wrench::StandardJob>, std::pair<wrench::WorkloadTask*, wrench::WorkloadTask*>> job_first_last_tasks;
    /** @brief Filename for the output-dump file */
    std::string filename;
    /** @brief Output filestream object to write out dump */
    std::ofstream filedump;

    /** @rief The number of jobs that have been submitted but haven't finished/failed yet **/
    size_t num_jobs_in_flight = 0;

    /** @brief number of complete jobs so far **/
    size_t num_completed_jobs = 0;

    /** @brief time to wait before submission **/
    double arrival_time = 0.;

    /** @brief job type for this workload*/
    WorkloadType workload_type;

    /** @brief switch to shuffle jobs **/
    bool shuffle_jobs = false;

    /** @brief generator to shuffle jobs **/
    std::mt19937 generator;
};

#endif//DCSIM_WORKLOAD_EXECUTION_CONTROLLER_H
