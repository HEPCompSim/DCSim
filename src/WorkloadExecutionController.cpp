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
#include <algorithm>
#include <memory>
#include "util/DefaultValues.h"

#include "WorkloadExecutionController.h"
#include "JobSpecification.h"
#include "computation/StreamedComputation.h"
#include "computation/CopyComputation.h"
#include "MonitorAction.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms, "Log category for WorkloadExecutionController");


/**
 *  @brief A simple ExecutionController building jobs from job-specifications, 
 *  submitting them and monitoring their execution
 * 
 *  @param workload_spec collection of job specifications
 *  @param job_scheduler A job scheduler
 *  @param grid_storage_services GRID storages holding files "for ever"
 *  @param cache_storage_services local caches evicting files when needed
 *  @param hostname host running the execution controller
 *  @param outputdump_name name of the file where the simulation's job information is stored
 *  @param shuffle_jobs switch to shuffle jobs for submission
 *  @param generator generator for job shuffling
 *  
 */
WorkloadExecutionController::WorkloadExecutionController(
        const Workload &workload_spec,
        const std::shared_ptr<JobScheduler> &job_scheduler,
        const std::set<std::shared_ptr<wrench::StorageService>> &grid_storage_services,
        const std::set<std::shared_ptr<wrench::StorageService>> &cache_storage_services,
        const std::string &hostname,
        const std::string &outputdump_name,
        const bool &shuffle_jobs, const std::mt19937 &generator) : wrench::ExecutionController(hostname,
                                                                                               "condor-simple") {
    for (auto &job_spec: workload_spec.job_batch) {
        this->workload_spec[job_spec.jobid] = job_spec;
    }
    this->arrival_time = workload_spec.submit_arrival_time;
    this->workload_type = workload_spec.workload_type;
    this->job_scheduler = job_scheduler;
    this->grid_storage_services = grid_storage_services;
    this->cache_storage_services = cache_storage_services;
    this->filename = outputdump_name;
    this->shuffle_jobs = shuffle_jobs;
    this->generator = generator;
}

/**
 * @brief Method to create a submit a job
 * @param job_name: the name of the job in the workload description
 * @param cs: the compute service on which to submit the job
 * @return the submitted job
 */
std::shared_ptr<wrench::CompoundJob> WorkloadExecutionController::createAndSubmitJob(const std::string &job_name,
                                                                                     const std::shared_ptr<wrench::ComputeService> &cs) {
    auto job_spec = this->workload_spec[job_name];
    auto job = job_manager->createCompoundJob(job_name);

    // Combined read-input-file-and-run-computation actions
    std::shared_ptr<MonitorAction> run_action;
    std::shared_ptr<wrench::ComputeAction> compute_action;
    if (this->workload_type == WorkloadType::Copy) {
        auto copy_computation = std::make_shared<CopyComputation>(
                this->cache_storage_services, this->grid_storage_services, job_spec.infiles, job_spec.total_flops);

        //? Split this into a caching file read and a standard compute action?
        // TODO: figure out what is the best value for the ability to parallelize HEP workloads on a CPU. Setting speedup to number of cores for now
        run_action = std::make_shared<MonitorAction>(
                "copycompute_" + job_name,
                job_spec.total_mem, job_spec.cores,
                *copy_computation,
                [](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {
                    WRENCH_INFO("Copy computation terminating");
                });
        job->addCustomAction(run_action);
    } else if (this->workload_type == WorkloadType::Streaming) {
        auto streamed_computation = std::make_shared<StreamedComputation>(
                this->cache_storage_services, this->grid_storage_services, job_spec.infiles, job_spec.total_flops,
                SimpleSimulator::prefetching_on);

        // TODO: figure out what is the best value for the ability to parallelize HEP workloads on a CPU. Setting speedup to number of cores for now
        run_action = std::make_shared<MonitorAction>(
                "streaming_" + job_name,
                job_spec.total_mem, job_spec.cores,
                *streamed_computation,
                [](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {
                    WRENCH_INFO("Streaming computation terminating");
                    // Do nothing
                });
        job->addCustomAction(run_action);
    } else if (this->workload_type == WorkloadType::Calculation) {
        // TODO: figure out what is the best value for the ability to parallelize HEP workloads on a CPU. Setting speedup to number of cores for now
        compute_action = job->addComputeAction(
                "calculation_" + job_name,
                job_spec.total_flops, job_spec.total_mem,
                job_spec.cores, job_spec.cores,
                wrench::ParallelModel::CONSTANTEFFICIENCY(1.0));
    } else {
        throw std::runtime_error("WorkloadType::" + workload_type_to_string(this->workload_type) + "not implemented!");
    }

    // Create the file write action
    auto fw_action = job->addFileWriteAction(
            "file_write_" + job_name,
            job_spec.outfile_destination);
    // //TODO: Think of a determination of storage_service to hold output data
    // // auto fw_action = job->addCustomAction(
    // //     "file_write_" + *job_name,
    // //     job_spec->total_mem, 0,
    // //     [](std::shared_ptr<wrench::ActionExecutor> action_executor) {
    // //         // TODO: Which storage service should we write output on?
    // //         // TODO: Probably random selection is fine, or just a fixed
    // //         // TODO: one that's picked by the "user"?
    // //         // TODO: Write the file at once
    // //     },
    // //     [](std::shared_ptr<wrench::ActionExecutor> action_executor) {
    // //         WRENCH_INFO("Output file was successfully written!")
    // //         // Do nothing
    // //     }
    // // );

    // Add necessary dependencies
    if (this->workload_type == WorkloadType::Streaming || this->workload_type == WorkloadType::Copy) {
        job->addActionDependency(run_action, fw_action);
    } else if (this->workload_type == WorkloadType::Calculation) {
        job->addActionDependency(compute_action, fw_action);
    }

    // Submit the job
    WRENCH_INFO("Submitting job %s to compute service %s...", job->getName().c_str(), cs->getName().c_str());
    job_manager->submitJob(job, cs);
    return job;
}


/**
 * @brief Remove a job specification from the workload because the corresponding job has been submitted
 * @param job_name: the name of the job in the workload description
 */
void WorkloadExecutionController::setJobSubmitted(const std::string &job_name) {
    // Remove it form the workload spec
    this->workload_spec_submitted[job_name] = this->workload_spec[job_name];
    this->workload_spec.erase(job_name);
}


/**
 * @brief Method to determine whether all jobs have been submitted
 * @return True is all jobs have been submitted, false otherwise
 */
bool WorkloadExecutionController::isWorkloadEmpty() const {
    return this->workload_spec.empty();
}


/**
 * @brief main method of the WorkloadExecutionController daemon
 * 
 * @return 0 on completion
 * 
 * @throw std::runtime_error
 */
int WorkloadExecutionController::main() {

    wrench::TerminalOutput::setThisProcessLoggingColor(wrench::TerminalOutput::COLOR_GREEN);

    WRENCH_INFO("Starting on host %s", wrench::Simulation::getHostName().c_str());
    WRENCH_INFO("About to execute a workload of %lu jobs", this->workload_spec.size());


    // Create a job manager
    this->job_manager = this->createJobManager();
    WRENCH_INFO("Created a job manager");

    // Create a data movement manager
    // DEPRECATED
    // this->data_movement_manager = this->createDataMovementManager();
    // WRENCH_INFO("Created a data manager");

    // Shuffle jobs for submission
    std::vector<const std::string *> job_spec_keys;
    job_spec_keys.reserve(this->workload_spec.size());
    for (const auto &job_name_spec: this->workload_spec) {
        job_spec_keys.push_back(&(job_name_spec.first));
    }
    if (this->shuffle_jobs) {
        std::shuffle(job_spec_keys.begin(), job_spec_keys.end(), generator);
    }

    // Sleep until my arrival time
    wrench::Simulation::sleep(this->arrival_time);

    // Let myself known to the job scheduler
    this->job_scheduler->addExecutionController(this);

    WRENCH_INFO("There are %ld jobs to schedule at time %f", this->workload_spec.size(), this->arrival_time);

    // Main loop
    size_t total_num_jobs = this->workload_spec.size();
    while ((this->num_completed_jobs < total_num_jobs) && (!this->abort)) {

        // Invoke the scheduler
        this->job_scheduler->schedule();

        try {
            this->waitForAndProcessNextEvent();
        } catch (wrench::ExecutionException &e) {
            WRENCH_INFO("Error while getting next execution event (%s)... ignoring and trying again",
                        (e.getCause()->toString().c_str()));
            continue;
        }
    }

    wrench::Simulation::sleep(10);

    WRENCH_INFO("--------------------------------------------------------");
    if (this->workload_spec.empty()) {
        WRENCH_INFO("Workload execution on %s is complete!", this->getHostname().c_str());
    } else {
        WRENCH_INFO("Workload execution on %s is incomplete!", this->getHostname().c_str());
    }

    WRENCH_INFO("WorkloadExecutionController daemon started on host %s terminating",
                wrench::Simulation::getHostName().c_str());

    this->job_manager.reset();

    return 0;
}


/**
 * @brief Process a ExecutionEvent::COMPOUND_JOB_FAILURE
 * Abort simulation once there is a failure.
 * 
 * @param event: an execution event
 */
void
WorkloadExecutionController::processEventCompoundJobFailure(const std::shared_ptr<wrench::CompoundJobFailedEvent> &event) {
    WRENCH_INFO("Notified that compound job %s has failed!", event->job->getName().c_str());
    WRENCH_INFO("Failure cause: %s", event->failure_cause->toString().c_str());
    WRENCH_INFO("As a WorkloadExecutionController, I abort as soon as there is a failure");
    this->num_completed_jobs++;
    this->abort = true;
}


/**
* @brief Process a ExecutionEvent::COMPOUND_JOB_COMPLETION.
* This also writes out a dump of job information returned by the simulation.
*
* @param event: an execution event
*/
void WorkloadExecutionController::processEventCompoundJobCompletion(
        const std::shared_ptr<wrench::CompoundJobCompletedEvent> &event) {

    this->job_scheduler->jobDone(event->job);
    this->num_completed_jobs++;

    auto job_name = event->job->getName();
    auto job_spec = this->workload_spec_submitted[job_name];
    this->workload_spec_submitted.erase(job_name); // clean up memory

    /* Retrieve the job that this event is for */
    WRENCH_INFO("Notified that job %s with %ld actions has completed", job_name.c_str(),
                event->job->getActions().size());

    /* Figure out execution host. All actions run on the same host, so let's just pick an arbitrary one */
    std::string execution_host = (*(event->job->getActions().begin()))->getExecutionHistory().top().physical_execution_host;

    /* Remove all actions from memory and compute incremental output values in one loop */
    double incr_compute_time = DefaultValues::UndefinedDouble;
    double incr_infile_transfertime = 0.;
    sg_size_t incr_infile_size = 0.;
    double incr_outfile_transfertime = 0.;
    sg_size_t incr_outfile_size = 0.;
    double global_start_date = DBL_MAX;
    double global_end_date = DBL_MIN;
    double hitrate = DefaultValues::UndefinedDouble;
    double flops = 0.;

    bool found_computation_action = false;

    // Figure out timings
    for (auto const &action: event->job->getActions()) {
        double start_date = action->getStartDate();
        double end_date = action->getEndDate();
        global_start_date = std::min<double>(global_start_date, start_date);
        global_end_date = std::max<double>(global_end_date, end_date);
        if (start_date < 0. || end_date < 0.) {
            throw std::runtime_error(
                    "Start date " + std::to_string(start_date) +
                    " or end date " + std::to_string(end_date) +
                    " of action " + action->getName() + " out of scope!");
        }
        double elapsed = end_date - start_date;
        WRENCH_DEBUG("Analyzing action: %s, started in s: %.2f, ended in s: %.2f, elapsed in s: %.2f",
                     action->getName().c_str(), start_date, end_date, elapsed);

        flops += job_spec.total_flops;
        if (auto file_read_action = std::dynamic_pointer_cast<wrench::FileReadAction>(action)) {
            incr_infile_transfertime += elapsed;
        } else if (auto monitor_action = std::dynamic_pointer_cast<MonitorAction>(action)) {
            if (found_computation_action) {
                throw std::runtime_error("There was more than one computation action in job " + job_name);
            }
            found_computation_action = true;
            if (incr_infile_transfertime <= 0. && incr_compute_time < 0. && hitrate < 0.) {
                incr_infile_transfertime = monitor_action->get_infile_transfer_time();
                incr_compute_time = monitor_action->get_calculation_time();
                hitrate = monitor_action->get_hitrate();
            } else {
                throw std::runtime_error(
                        "Some of the job information for action " + monitor_action->getName() +
                        " has already been filled. Abort!");
            }
        } else if (auto file_write_action = std::dynamic_pointer_cast<wrench::FileWriteAction>(action)) {
            if (end_date >= start_date) {
                incr_outfile_transfertime += end_date - start_date;
            } else {
                throw std::runtime_error(
                        "Writing outputfile " + job_spec.outfile->getID() +
                        " for job " + job_name + " finished before start!");
            }
        } else if (auto compute_action = std::dynamic_pointer_cast<wrench::ComputeAction>(action)) {
            if (end_date >= start_date) {
                if (incr_compute_time == DefaultValues::UndefinedDouble) {
                    incr_compute_time = end_date - start_date;
                } else {
                    incr_compute_time += end_date - start_date;
                }
            } else {
                throw std::runtime_error(
                        "Computation for job " + job_name + " finished before start!");
            }
        }
    }

    // Figure out file sizes
    for (auto const &f: job_spec.infiles) {
        incr_infile_size += f->getSize();
    }
    incr_outfile_size += job_spec.outfile->getSize();

    /* Dump relevant information to file */
    this->filedump.open(this->filename, ios::out | ios::app);
    if (this->filedump.is_open()) {

        this->filedump << job_name << ", ";
        // << std::to_string(job->getMinimumRequiredNumCores()) << ", "
        // << std::to_string(job->getMinimumRequiredMemory()) << ", "
        // << /*TODO: find a way to get disk usage on scratch space */ << ", ";
        this->filedump << execution_host << ", " << hitrate << ", ";
        this->filedump << std::to_string(global_start_date) << ", " << std::to_string(global_end_date) << ", ";
        this->filedump << std::to_string(incr_compute_time) << ", " << std::to_string(flops) << ", ";
        this->filedump << std::to_string(incr_infile_transfertime) << ", " << std::to_string(incr_infile_size) << ", ";
        this->filedump << std::to_string(incr_outfile_transfertime) << ", " << std::to_string(incr_outfile_size)
                       << std::endl;

        this->filedump.close();

        WRENCH_INFO("Information for job %s has been dumped into file %s", job_name.c_str(), this->filename.c_str());
    } else {
        throw std::runtime_error("Couldn't open output-file " + this->filename + " for dump!");
    }
}
