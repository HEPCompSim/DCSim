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

#include "SimpleExecutionController.h"
#include "JobSpecification.h"
#include "computation/StreamedComputation.h"
#include "computation/CopyComputation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms, "Log category for SimpleExecutionController");

/**
 *  @brief A simple ExecutionController building jobs from job-specifications, 
 *  submitting them and monitoring their execution
 * 
 *  @param workload_spec collection of job specifications
 *  @param htcondor_compute_services collection of HTCondorComputeServices submitting jobs
 *  @param grid_storage_services GRID storages holding files "for ever"
 *  @param cache_storage_services local caches evicting files when needed
 *  @param hostname host running the execution controller
 *  @param outputdump_name name of the file where the simulation's job information is stored
 *  
 */
SimpleExecutionController::SimpleExecutionController(
        const std::map<std::string, JobSpecification>& workload_spec,
        const std::set<std::shared_ptr<wrench::HTCondorComputeService>>& htcondor_compute_services,
        const std::set<std::shared_ptr<wrench::StorageService>>& grid_storage_services,
        const std::set<std::shared_ptr<wrench::StorageService>>& cache_storage_services,
        const std::string& hostname,
        const std::string& outputdump_name) : wrench::ExecutionController(
        hostname,
        "condor-simple") {
    this->workload_spec = workload_spec;
    this->htcondor_compute_services = htcondor_compute_services;
    this->grid_storage_services = grid_storage_services;
    this->cache_storage_services = cache_storage_services;
    this->filename = outputdump_name;
}

/**
 * @brief main method of the SimpleExecutionController daemon
 * 
 * @return 0 on completion
 * 
 * @throw std::runtime_error
 */
int SimpleExecutionController::main() {

    wrench::TerminalOutput::setThisProcessLoggingColor(wrench::TerminalOutput::COLOR_GREEN);

    /* initialize output-dump file */
    this->filedump.open(this->filename, ios::out | ios::trunc);
    if (this->filedump.is_open()) {
        this->filedump << "job.tag" << ",\t"; // << "job.ncpu" << ",\t" << "job.memory" << ",\t" << "job.disk" << ",\t";
        this->filedump << "machine.name" << ",\t";
        this->filedump << "job.start" << ",\t" << "job.end" << ",\t" << "job.computetime" << ",\t";
        this->filedump << "infiles.transfertime" << ",\t" << "infiles.size" << ",\t" << "outfiles.transfertime" << ",\t" << "outfiles.size" << std::endl;
        this->filedump.close();

        WRENCH_INFO("Wrote header of the output dump into file %s", this->filename.c_str());
    }
    else {
        throw std::runtime_error("Couldn't open output-file " + this->filename + " for dump!");
    }

    WRENCH_INFO("Starting on host %s", wrench::Simulation::getHostName().c_str());
    WRENCH_INFO("About to execute a workload of %lu jobs", workload_spec.size());


    // Create a job manager
    this->job_manager = this->createJobManager();
    WRENCH_INFO("Created a job manager");

    // Create a data movement manager
    this->data_movement_manager = this->createDataMovementManager();
    WRENCH_INFO("Created a data manager");


    // Get the available compute services
    // TODO: generalize to arbitrary numbers of HTCondorComputeServices
    if (this->htcondor_compute_services.empty()) {
        throw std::runtime_error("Aborting - No compute services available!");
    }
    if (this->htcondor_compute_services.size() != 1) {
        throw std::runtime_error("This execution controller running on " + this->getHostname() + " requires a single HTCondorCompute service");
    }
    WRENCH_INFO("Found %ld HTCondor Service(s) on:", this->htcondor_compute_services.size());
    for (auto htcondor_compute_service: this->htcondor_compute_services) {
        WRENCH_INFO("\t%s", htcondor_compute_service->getHostname().c_str());
    }
    auto htcondor_compute_service = *this->htcondor_compute_services.begin();


    // Create and submit all the jobs!
    WRENCH_INFO("There are %ld jobs to schedule", this->workload_spec.size());
    for (auto job_name_spec: this->workload_spec) {
        std::string job_name = job_name_spec.first;
        auto job_spec = &this->workload_spec[job_name];

        auto job = job_manager->createCompoundJob(job_name);

        // Combined read-input-file-and-run-computation actions
        std::shared_ptr<wrench::CustomAction> run_action;
        if (! SimpleSimulator::use_blockstreaming) {
            auto copy_computation = std::shared_ptr<CopyComputation>(
                new CopyComputation(this->cache_storage_services, this->grid_storage_services, job_spec->infiles, job_spec->total_flops)
            );

            //? Split this into a caching file read and a standard compute action?
            run_action = job->addCustomAction(
                "copycompute_" + job_name,
                job_spec->total_mem, 1,
                *copy_computation,
                [](std::shared_ptr<wrench::ActionExecutor> action_executor) {
                    WRENCH_INFO("Copy computation done")
                }
            );
        } else {
            auto streamed_computation = std::shared_ptr<StreamedComputation>(
                new StreamedComputation(this->cache_storage_services, this->grid_storage_services, job_spec->infiles, job_spec->total_flops)
            );

            run_action = job->addCustomAction(
                "streaming_" + job_name,
                job_spec->total_mem, 1,
                *streamed_computation,
                [](std::shared_ptr<wrench::ActionExecutor> action_executor) {
                    WRENCH_INFO("Streaming computation done");
                    // Do nothing
                }
            );
        }

        // Create the file write action
        auto fw_action = job->addFileWriteAction(
            "file_write_" + job_name,
            job_spec->outfile,
            job_spec->outfile_destination
        );
        //TODO: Think of a determination of storage_service to hold output data
        // auto fw_action = job->addCustomAction(
        //     "file_write_" + job_name,
        //     job_spec->total_mem, 0,
        //     [](std::shared_ptr<wrench::ActionExecutor> action_executor) {
        //         // TODO: Which storage service should we write output on?
        //         // TODO: Probably random selection is fine, or just a fixed
        //         // TODO: one that's picked by the "user"?
        //         // TODO: Write the file at once
        //     },
        //     [](std::shared_ptr<wrench::ActionExecutor> action_executor) {
        //         WRENCH_INFO("Output file was successfully written!")
        //         // Do nothing
        //     }
        // );

        // Add necessary dependencies
        job->addActionDependency(run_action, fw_action);

        // Submit the job for execution!
        //TODO: generalize to arbitrary numbers of htcondor services
        job_manager->submitJob(job, htcondor_compute_service);
        WRENCH_INFO("Submitted job %s", job->getName().c_str());

    }

    WRENCH_INFO(
        "Job manager %s: Done with creation/submission of all compound jobs on host %s", 
        job_manager->getName().c_str(), job_manager->getHostname().c_str()
    );


    this->num_completed_jobs = 0;
    while (this->num_completed_jobs != this->workload_spec.size()) {
        // Wait for a workflow execution event, and process it
        try {
            this->waitForAndProcessNextEvent();
        } catch (wrench::ExecutionException &e) {
            WRENCH_INFO("Error while getting next execution event (%s)... ignoring and trying again", (e.getCause()->toString().c_str()));
            continue;
        }

        if (this->abort || this->num_completed_jobs == this->workload_spec.size()) {
            break;
        }
    }

    wrench::Simulation::sleep(10);

    WRENCH_INFO("--------------------------------------------------------")
    if (this->num_completed_jobs == this->workload_spec.size()){
        WRENCH_INFO("Workload execution on %s is complete!", this->getHostname().c_str());
    } else{
        WRENCH_INFO("Workload execution on %s is incomplete!", this->getHostname().c_str());
    }

    WRENCH_INFO("SimpleExecutionController daemon started on host %s terminating", wrench::Simulation::getHostName().c_str());

    this->job_manager.reset();

    return 0;
}


/**
 * @brief Process a ExecutionEvent::COMPOUND_JOB_FAILURE
 * Abort simulation once there is a failure.
 * 
 * @param event: an execution event
 */
void SimpleExecutionController::processEventCompoundJobFailure(std::shared_ptr<wrench::CompoundJobFailedEvent> event) {
    WRENCH_INFO("Notified that compound job %s has failed!", event->job->getName().c_str());
    WRENCH_INFO("Failure cause: %s", event->failure_cause->toString().c_str());
    WRENCH_INFO("As a SimpleExecutionController, I abort as soon as there is a failure");
    this->abort = true;
}


/**
* @brief Process a ExecutionEvent::COMPOUND_JOB_COMPLETION.
* This also writes out a dump of job information returned by the simulation.
*
* @param event: an execution event
*/
void SimpleExecutionController::processEventCompoundJobCompletion(std::shared_ptr<wrench::CompoundJobCompletedEvent> event) {

    /* Retrieve the job that this event is for */
    WRENCH_INFO("Notified that job %s with %ld actions has completed", event->job->getName().c_str(), event->job->getActions().size());

    this->num_completed_jobs++;

    /* Figure out execution host. All actions run on the same host, so let's just pick an arbitrary one */
    std::string execution_host = (*(event->job->getActions().begin()))->getExecutionHistory().top().physical_execution_host;

    /* Remove all actions from memory and compute incremental output values in one loop */
    double incr_compute_time = 0.;
    double incr_infile_transfertime = 0.;
    double incr_infile_size = 0.;
    double incr_outfile_transfertime = 0.;
    double incr_outfile_size = 0.;
    double start_date = DBL_MAX;
    double end_date = 0;

    // Figure out timings
    for (auto const &action : event->job->getActions()) {
        double elapsed = action->getEndDate() - action->getStartDate();
        WRENCH_DEBUG("Running action: %s, elapsed in s: %.2f", action->getName().c_str(), elapsed);
        start_date = std::min<double>(start_date, action->getStartDate());
        end_date = std::max<double>(end_date, action->getEndDate());
        // TODO: Better: Check for action type rather than doing string matching
        if (action->getName().find("file_read_") != std::string::npos) {
            incr_infile_transfertime += elapsed;
        } else if (action->getName().find("copycompute_") != std::string::npos || action->getName().find("streaming_") != std::string::npos) {
            incr_compute_time += elapsed;
        } else if (action->getName().find("file_write_") != std::string::npos) {
            incr_outfile_transfertime += elapsed;
        }
    }

    // Figure out file sizes
    for (auto const &f : this->workload_spec[event->job->getName()].infiles) {
        incr_infile_size += f->getSize();
    }
    incr_outfile_size += this->workload_spec[event->job->getName()].outfile->getSize();


    /* Dump relevant information to file */
    this->filedump.open(this->filename, ios::out | ios::app);
    if (this->filedump.is_open()) {

        this->filedump << event->job->getName() << ",\t"; //<< std::to_string(job->getMinimumRequiredNumCores()) << ",\t" << std::to_string(job->getMinimumRequiredMemory()) << ",\t" << /*TODO: find a way to get disk usage on scratch space */ << ",\t" ;
        this->filedump << execution_host << ",\t";
        this->filedump << std::to_string(start_date) << ",\t" << std::to_string(end_date) << ",\t" << std::to_string(incr_compute_time) << ",\t" << std::to_string(incr_infile_transfertime) << ",\t" ;
        this->filedump << std::to_string(incr_infile_size) << ",\t" << std::to_string(incr_outfile_transfertime) << ",\t" << std::to_string(incr_outfile_size) << std::endl;

        this->filedump.close();

        WRENCH_INFO("Information for job %s has been dumped into file %s", event->job->getName().c_str(), this->filename.c_str());
    }
    else {
        throw std::runtime_error("Couldn't open output-file " + this->filename + " for dump!");
    }

}
