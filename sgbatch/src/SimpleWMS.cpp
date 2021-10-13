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
    WRENCH_INFO("About to execute a workflow with %lu tasks", this->getWorkflow()->getNumberOfTasks());

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
    WRENCH_INFO("Found %ld HTCondor Service(s) on %s", htcondor_compute_services.size(), htcondor_compute_service->getHostname().c_str());

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

    while (not this->getWorkflow()->isDone()) {

        // Get the ready tasks
        std::vector<wrench::WorkflowTask *> ready_tasks = this->getWorkflow()->getReadyTasks();

        WRENCH_INFO("There are %ld ready tasks to schedule", ready_tasks.size());
        for (auto task : ready_tasks) {
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

            // Locate inputfiles on all caches until hitrate threshold is reached
            // and locate them also at remote storages as fallback
            int counter = 0;
            for (auto f : task->getInputFiles()) {
                for (auto remote_storage : remote_storage_services) {
                    file_locations.insert(std::make_pair(f, wrench::FileLocation::LOCATION(remote_storage)));
                }
                //TODO: figure out how to prefetch the inputfiles on caches
                if ((incr_inputfile_sizes.at(counter)/incr_inputfile_sizes.back()) >= this->hitrate) continue;
                for (auto cache : worker_storage_services) {
                    file_locations.insert(std::make_pair(f, wrench::FileLocation::LOCATION(cache)));
                }
            }
            //TODO: distribute inputfiles accordingly on the corresponding storage services
            //TODO: ensure that inutfiles are read preferably from cache

            // Set locations for outputfiles to remote storage
            for (auto f : task->getOutputFiles()) {
                //TODO: generalize to arbitrary numbers of remote storages
                file_locations.insert(std::make_pair(f, wrench::FileLocation::LOCATION(remote_storage_service)));
            }

            auto job = this->job_manager->createStandardJob(task, file_locations);
            std::map<std::string, std::string> htcondor_service_specific_args = {};
            //TODO: generalize to arbitrary numbers of htcondor services
            this->job_manager->submitJob(job, htcondor_compute_service, htcondor_service_specific_args);
        }
        WRENCH_INFO("Done with scheduling tasks as standard jobs");

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
