/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <iostream>

#include "SimpleWMS.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms, "Log category for Simple WMS");

/**
 * @brief Create a Simple WMS with a workflow instance, a scheduler implementation, and a list of compute services
 */
SimpleWMS::SimpleWMS(std::unique_ptr<wrench::StandardJobScheduler> standard_job_scheduler,
                     std::unique_ptr<wrench::PilotJobScheduler> pilot_job_scheduler,
                     const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                     const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                     const std::string &hostname) : wrench::WMS(
         std::move(standard_job_scheduler),
         std::move(pilot_job_scheduler),
         compute_services,
         storage_services,
         {},
         nullptr,
         hostname,
         "condor-simple") {}

/**
 * @brief main method of the SimpleWMS daemon
 * 
 * @return 0 on completion
 * 
 * @throw std::runtime_error
 */
int SimpleWMS::main() {

  wrench::TerminalOutput::setThisProcessLoggingColor(wrench::TerminalOutput::COLOR_GREEN);

  // Check whether the WMS has a deferred start time
  checkDeferredStart();

  WRENCH_INFO("Starting on host %s", wrench::Simulation::getHostName().c_str());
  WRENCH_INFO("About to execute a workflow with %lu tasks", this->getWorkflow()->getNumberOfTasks());

  // Create a job manager
  this->job_manager = this->createJobManager();

  // Create a data movement manager
  this->data_movement_manager = this->createDataMovementManager();


  while (not this->getWorkflow()->isDone()) {
    // Get the ready tasks
    std::vector<wrench::WorkflowTask *> ready_tasks = this->getWorkflow()->getReadyTasks();

    // Get the available compute services
    auto htcondor_compute_services = this->getAvailableComputeServices<wrench::HTCondorComputeService>();

    if (htcondor_compute_services.empty()) {
      WRENCH_INFO("Aborting - No compute services available!");
      break;
    }

    //Submit pilot jobs
    if (this->getPilotJobScheduler()) {
      WRENCH_INFO("Scheduling pilot jobs...");
      this->getPilotJobScheduler()->schedulePilotJobs(this->getAvailableComputeServices<wrench::ComputeService>());
    }
    
    // Run ready tasks with defined scheduler implementation
    WRENCH_INFO("Scheduling tasks...");
    this->getStandardJobScheduler()->scheduleTasks(htcondor_compute_services, ready_tasks);

    // Wait for a workflow execution event, and process it
    try {
      this->waitForAndProcessNextEvent();
    } catch (wrench::WorkflowExecutionException &e) {
      WRENCH_INFO("Error while getting next execution event (%s)... ignoring and trying again",
                   (e.getCause()->toString().c_str()));
      continue;
    }

    if (this->abort || this->getWorkflow()->isDone()) {
      break;
    }
  }

  wrench::Simulation::sleep(10);

  WRENCH_INFO("--------------------------------------------------------")
  if (this->getWorkflow()->isDone()){
    WRENCH_INFO("Workflow execution is complete!");
  } else{
    WRENCH_INFO("Workflow execution is incomplete!")
  }
   
  WRENCH_INFO("Simple WMS daemon started on host %s terminating", wrench::Simulation::getHostName().c_str());

  this->job_manager.reset();

  return 0;
}

/**
 * @brief Process a WorkflowExecutionEvent::STANDARD_JOB_FAILURE
 * 
 * @param event: a workflow execution event
 */
void SimpleWMS::processEventStandardJobFailure(std::shared_ptr<wrench::StandardJobFailedEvent> event) {
  /* Retrieve the job that this event is for */
  auto job = event->standard_job;
  WRENCH_INFO("Notified that a standard job has failed (all its tasks are back in the ready state)");
  WRENCH_INFO("CauseType: %s", event->failure_cause->toString().c_str());
  WRENCH_INFO("As a SimpleWMS, I abort as soon as there is a failure");
  this->abort = true;
}
