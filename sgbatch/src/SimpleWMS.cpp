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
 * @brief Helper function to recursively retrieve all descendants of a WorkflowTask recursively. 
 * The returned vector is unfiltered and contains duplicates, which have to be removed.
 * 
 * @param task: a WorkflowTask
 * 
 * @return vector of WorkflowTask pointers to duplicated descendant tasks
 * 
 * @throw std::invalid_argument
 */
std::vector<wrench::WorkflowTask*> getUnfilteredDescendants(const wrench::WorkflowTask* task) {
  if (task == nullptr) {
    throw std::invalid_argument("getDescendants(): Invalid arguments");
  }
  // Recursively get all children and children's children
  std::vector<wrench::WorkflowTask*> descendants;
  auto children = task->getChildren();
  if (!children.empty()) {
    descendants.insert(descendants.end(), children.begin(), children.end());
  }
  for (auto child : children) {
    auto tmp_descendants = getUnfilteredDescendants(child);
    if (!tmp_descendants.empty()) {
      descendants.insert(descendants.end(), tmp_descendants.begin(), tmp_descendants.end());
    }
  }

  return descendants;
}

/**
 * @brief Helper function to retrieve a vector of unique descendant WorkflowTasks of a common ancestor
 * 
 * @param patriarch: common ancestor WorkflowTask of the chain
 * 
 * @return filtered vector of WorkflowTask pointers to descendant tasks
 * 
 * @throw std::invalid_argument
 */
std::vector<wrench::WorkflowTask*> getDescendants(const wrench::WorkflowTask* patriarch) {
  // get unfiltered vector of descendants
  auto descendants = getUnfilteredDescendants(patriarch);
  // Filter duplicates
  std::sort(descendants.begin(), descendants.end());
  auto last = std::unique(descendants.begin(), descendants.end());
  descendants.erase(last, descendants.end()); 

  return descendants;
}


/**
 * @brief Create a Simple WMS with a workflow instance, a list of storage services and a list of compute services
 */
SimpleWMS::SimpleWMS(const std::set<std::shared_ptr<wrench::ComputeService>>& compute_services,
                     const std::set<std::shared_ptr<wrench::StorageService>>& storage_services,
                     const std::set<std::shared_ptr<wrench::NetworkProximityService>>& network_proximity_services,
                     std::shared_ptr<wrench::FileRegistryService> file_registry_service,
                     const std::string& hostname,
                     const double& hitrate) : wrench::WMS(
        nullptr, nullptr,
        compute_services,
        storage_services,
        network_proximity_services,
        file_registry_service,
        hostname,
        "condor-simple") {
    this->hitrate = hitrate;
}

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
    WRENCH_INFO("About to execute a workflow with %lu tasks", 
                this->getWorkflow()->getNumberOfTasks());


    // Create a job manager
    this->job_manager = this->createJobManager();
    WRENCH_INFO("Created a job manager");

    // Create a data movement manager
    this->data_movement_manager = this->createDataMovementManager();
    WRENCH_INFO("Created a data manager");


    // Get the available compute services
    // TODO: generalize to arbitrary numbers of HTCondorComputeServices
    auto htcondor_compute_services = this->getAvailableComputeServices<wrench::HTCondorComputeService>();
    if (htcondor_compute_services.empty()) {
        throw std::runtime_error("Aborting - No compute services available!");
    }
    if (htcondor_compute_services.size() != 1) {
        throw std::runtime_error("This example Simple HTCondor Scheduler requires a single compute service");
    }

    auto htcondor_compute_service = *htcondor_compute_services.begin();
    WRENCH_INFO("Found %ld HTCondor Service(s) on %s", 
                htcondor_compute_services.size(), 
                htcondor_compute_service->getHostname().c_str());


    // Get the available storage services
    auto storage_services = this->getAvailableStorageServices();
    // and split between workers and remote storages
    std::set<std::shared_ptr<wrench::StorageService>> worker_storage_services;
    std::set<std::shared_ptr<wrench::StorageService>> remote_storage_services;
    for (auto storage : storage_services) {
        std::string hostname = storage->getHostname();
        std::for_each(hostname.begin(), hostname.end(), [](char& c){c = std::tolower(c);});
        if (hostname.find("remote") != std::string::npos) {
            remote_storage_services.insert(storage);
        } else {
            worker_storage_services.insert(storage);
        }
    }
    // Check that the right remote_storage_service is passed for outputfile storage
    // TODO: generalize to arbitrary numbers of remote storages
    if (remote_storage_services.size() != 1) {
        throw std::runtime_error("This example Simple Simulator requires a single remote_storage_service");
    }
    auto remote_storage_service = *remote_storage_services.begin();
    WRENCH_INFO("Found %ld Remote Storage Service(s) on %s", remote_storage_services.size(), remote_storage_service->getHostname().c_str());


    // Get the entry tasks
    std::vector<wrench::WorkflowTask*> entry_tasks = this->getWorkflow()->getEntryTasks();
    WRENCH_INFO("There are %ld task-chains to schedule", entry_tasks.size());
    //TODO: check if #entry-taks=#jobs=#chains
    // std::cerr << "There are " << std::to_string(entry_tasks.size()) << " task-chains to schedule" << std::endl;


    // Group task chunks belonging to the same task-chain into single job
    std::pair<wrench::WorkflowTask *, wrench::WorkflowTask *> first_last_tasks;

    int counter = 0;
    for (auto entry_task : entry_tasks) {
        // TODO: first_last_tasks.first = ???
        // TODO: first_last_tasks.second = ???
      std::vector<wrench::WorkflowTask*> task_chunks;
      // if (!task_chunks.empty()) task_chunks.clear();
      counter += 1;
      // std::cerr << "Chain number " << std::to_string(counter) << " contains these tasks:" << std::endl;
      // Group all tasks of the same chain
      // starting with the entry task
      task_chunks.push_back(entry_task);
      // and add all children and children's children
      auto descendants = getDescendants(entry_task);  
      task_chunks.insert(task_chunks.end(), descendants.begin(), descendants.end());
      // std::cerr << "(In the chain are " << std::to_string(task_chunks.size()) << " tasks in total)" << std::endl;

      // Identify file-locations on storage services
      std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;     
      for (auto task : task_chunks) {
        // std::cerr << "\t" << task->getID().c_str() << std::endl;
        // Identify input-file locations
        for (auto f : task->getInputFiles()) {
          // Fill vector of input-file locations in order of read-priority
          std::vector<std::shared_ptr<wrench::FileLocation>> locations;
          //! Caches have highest priority -> Caches are added in HTCondorNegotiator
          //! only give remote file locations, local caches will be filled by caching functionality
          // for (auto cache : worker_storage_services) {
          //   locations.insert(locations.end(), wrench::FileLocation::LOCATION(cache));
          // }
          // Remote storages are fallback
          for (auto remote_storage : remote_storage_services) {
            locations.insert(locations.end(), wrench::FileLocation::LOCATION(remote_storage));
          }
          file_locations[f] = locations;
        }
        // Identify output-file locations
        for (auto f : task->getOutputFiles()) {
          // Fill vector of output-file locations in order of write-priority
          std::vector<std::shared_ptr<wrench::FileLocation>> locations;
          // Write to first remote storage
          //TODO: chose the output file location according to a logic? 
          for (auto remote_storage : remote_storage_services) {
            locations.insert(locations.end(), wrench::FileLocation::LOCATION(remote_storage));
          }
          file_locations[f] = locations;
        }
      }

      // Create and submit a job for each chain
      auto job = this->job_manager->createStandardJob(task_chunks, file_locations);
      this->job_first_last_tasks[job] = first_last_tasks;
      std::map<std::string, std::string> htcondor_service_specific_args = {};
      //TODO: generalize to arbitrary numbers of htcondor services
      this->job_manager->submitJob(job, htcondor_compute_service, htcondor_service_specific_args);
    }
    WRENCH_INFO("Done with scheduling tasks as standard jobs");


    while (not this->getWorkflow()->isDone()) {
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

    WRENCH_INFO("Simple WMS daemon started on host %s terminating", 
                wrench::Simulation::getHostName().c_str());

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

/**
* @brief Process a WorkflowExecutionEvent::STANDARD_JOB_COMPLETION
*
* @param event: a workflow execution event
*/
void SimpleWMS::processEventStandardJobCompletion(std::shared_ptr<wrench::StandardJobCompletedEvent> event) {

    /* Retrieve the job that this event is for */
    auto job = event->standard_job;

    /* Identify first/last tasks */
    auto first_task = std::get<0>(this->job_first_last_tasks[job]);
    auto last_task = std::get<1>(this->job_first_last_tasks[job]);

    // TODO: Extract/save relevant information

    /* Remove all tasks */
    for (auto const &task: job->getTasks()) {
        this->getWorkflow()->removeTask(task);
    }

    this->job_first_last_tasks.erase(job);

}
