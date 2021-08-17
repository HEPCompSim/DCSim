/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "SimpleHTCondorJobScheduler.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_scheduler, "Log category for Simple Scheduler");

/**
 * @brief Schedule and run a set of ready tasks on available resources
 *
 * @param compute_services: a set of compute services available to run jobs
 * @param tasks: a map of (ready) workflow tasks
 * @param hitrate: a double fraction of stored files on file caches
 *
 * @throw std::runtime_error
 */
void SimpleHTCondorJobScheduler::scheduleTasks(const std::set<std::shared_ptr<wrench::HTCondorComputeService>> & compute_services,
                                               const std::vector<wrench::WorkflowTask *> & tasks,
                                               const double & hitrate) {

  // Check that the right compute_service is passed
  if (compute_services.size() != 1) {
    throw std::runtime_error("This example Simple HTCondor Scheduler requires a single compute service");
  }
  auto compute_service = *compute_services.begin();
  std::shared_ptr<wrench::HTCondorComputeService> htcondor_service;
  if (not(htcondor_service = std::dynamic_pointer_cast<wrench::HTCondorComputeService>(compute_service))) {
    throw std::runtime_error("This example HTCondor Scheduler can only handle a single HTCondor service");
  }

  // Check that the right remote_storage_service is passed for outputfile storage
  if (remote_storage_services.size() != 1) {
    throw std::runtime_error("This example Simple Simulator requires a single remote_storage_service");
  }
  auto remote_storage_service = *remote_storage_services.begin();

  WRENCH_INFO("There are %ld ready tasks to schedule", tasks.size());
  for (auto task : tasks) {
    // Distribute input files to storage services according to hitrate value
    std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>> file_locations;

    // Compute the task's incremental inputfile sizes
    std::vector<double> incr_inputfile_sizes;
    for (auto f : task->getInputFiles()) {
      if (incr_inputfile_sizes.empty()){
        incr_inputfile_sizes.push_back(f->getSize());
      } else {
        incr_inputfile_sizes.push_back(f->getSize()+incr_inputfile_sizes.back());
      }
    }
    
    int counter = 0;
    for (auto f : task->getInputFiles()) {
      // Distribute the inputfiles on all caches untill hitrate threshold is reached
      for (auto cache : worker_storage_services) {
        file_locations.insert(std::make_pair(f, wrench::FileLocation::LOCATION(cache)));
      }
      if (incr_inputfile_sizes.at(counter)/incr_inputfile_sizes.back() > hitrate) break;
    }

    // Write outputfiles back to remote storage
    for (auto f : task->getOutputFiles()) {
      file_locations.insert(std::make_pair(f, wrench::FileLocation::LOCATION(remote_storage_service)));
    }

    auto job = this->getJobManager()->createStandardJob(task, file_locations);
    std::map<std::string, std::string> htcondor_job_args = {};
    this->getJobManager()->submitJob(job, htcondor_service, htcondor_job_args);
  }
  WRENCH_INFO("Done with scheduling tasks as standard jobs");
}
