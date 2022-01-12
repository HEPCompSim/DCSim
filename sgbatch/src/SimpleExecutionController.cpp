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
#include "StreamedComputation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms, "Log category for SimpleExecutionController");

#if 0 // THESE FUNCTIONS ARE NOT USED
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

#endif


/**
 * @brief Create a SimpleExecutionController with a workload specification instance, a list of storage services and a list of compute services
 *
 * @param workload_spec: the workload specification
 * @param htcondor_compute_service: an HTCondor compute service
 * @param storage_services: set of storage services holding input files //! currently only remote storages needed
 * @param hostname: host where the WMS runs
 * @param outputdump_name: name of the file to dump simulation information
 */
SimpleExecutionController::SimpleExecutionController(
        const std::map<std::string, JobSpecification> &workload_spec,
        const std::set<std::shared_ptr<wrench::HTCondorComputeService>>& htcondor_compute_services,
        const std::set<std::shared_ptr<wrench::StorageService>>& storage_services,
        const std::string& hostname,
        const std::string& outputdump_name) : wrench::ExecutionController(
        hostname,
        "condor-simple") {
    this->workload_spec = workload_spec;
    this->filename = outputdump_name;
    this->htcondor_compute_services = htcondor_compute_services;
    this->storage_services = storage_services;
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
        throw std::runtime_error("This example Simple HTCondor Scheduler requires a single compute service");
    }

    auto htcondor_compute_service = *(this->htcondor_compute_services.begin());
    WRENCH_INFO("Found %ld HTCondor Service(s) on %s", htcondor_compute_services.size(), htcondor_compute_service->getHostname().c_str());


    // Get the available storage services
    // and split between workers and remote storages
    std::set<std::shared_ptr<wrench::StorageService>> worker_storage_services;
    std::set<std::shared_ptr<wrench::StorageService>> remote_storage_services;
    for (auto storage : this->storage_services) {
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


    // Create and submit all the jobs!
    WRENCH_INFO("There are %ld jobs to schedule", this->workload_spec.size());
    for (ssize_t j = 0; j < this->workload_spec.size(); j++) {
        std::string job_name = "job_" + std::to_string(j);
        auto job_spec = &this->workload_spec[job_name];

        auto job = job_manager->createCompoundJob(job_name);

        // Read-Input file actions
        auto streamed_computation = std::shared_ptr<StreamedComputation>(new StreamedComputation(this->storage_services,

                                                                                                 job_spec->infiles));
        // TODO: ADD SOMETHING FOR THE MEMORY FOOTPRINT BELOW!
        // TODO: NOTE THAT WE USE ONLY ONE CORE, WHICH IS LIKELY OK? COULD DO MORE...
        auto streaming_action = job->addCustomAction("streaming_" + std::to_string(j),
                                                     0, 1,
                                                     *streamed_computation,
                                                     [](std::shared_ptr<wrench::ActionExecutor> action_executor) {
                                                         WRENCH_INFO("Streaming computation done");
                                                         // Do nothing
                                                     }
        );

        // Create the file write action
        auto fw_action = job->addCustomAction("file_write_" + std::to_string(j),
                                              0, 0,
                                              [](std::shared_ptr<wrench::ActionExecutor> action_executor) {
                                                  // TODO: Which storage service should we write output on?
                                                  // TODO: Probably random selection is fine, or just a fixed
                                                  // TODO: one that's picked by the "user"?
                                                  // TODO: Write the file at once
                                              },
                                              [](std::shared_ptr<wrench::ActionExecutor> action_executor) {
                                                  // Do nothing
                                              }
        );

        // Add necessary dependencies
        job->addActionDependency(streaming_action, fw_action);

        // Submit the job for execution!
        job_manager->submitJob(job, htcondor_compute_service);
        WRENCH_INFO("Submitted job %s", job->getName().c_str());

    }

    WRENCH_INFO("Done with creation/submission of all compound jobs");

    // BELOW IS OLD CODE
//
//    // Group task chunks belonging to the same task-chain into single job
//    for (auto entry_task : entry_tasks) {
//
//        // Group all tasks of the same chain
//        std::vector<wrench::WorkflowTask*> task_chunks;
//        // starting with the entry task
//        task_chunks.push_back(entry_task);
//        // and add all children and children's children
//        auto descendants = getDescendants(entry_task);
//        task_chunks.insert(task_chunks.end(), descendants.begin(), descendants.end());
//
//        // Identify first and last task of the job for output
//        std::pair<wrench::WorkflowTask *, wrench::WorkflowTask *> first_last_tasks;
//        first_last_tasks.first = entry_task;
//        bool found_last_task = false;
//        for (auto task : descendants) {
//            if (found_last_task) {
//                throw std::runtime_error("Found more than one last-task in the task chain starting with task " + entry_task->getID() + "!");
//            }
//            if (task->getNumberOfChildren() == 0) {
//                first_last_tasks.second = task;
//                found_last_task = true;
//            }
//        }
//
//        // Identify file-locations on storage services
//        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
//        for (auto task : task_chunks) {
//            // std::cerr << "\t" << task->getID().c_str() << std::endl;
//            // Identify input-file locations
//            for (auto f : task->getInputFiles()) {
//                // Fill vector of input-file locations in order of read-priority
//                std::vector<std::shared_ptr<wrench::FileLocation>> locations;
//                //! Caches have highest priority -> Caches are added in HTCondorNegotiator
//                //! only give remote file locations, local caches will be filled by caching functionality
//                // for (auto cache : worker_storage_services) {
//                //     locations.insert(locations.end(), wrench::FileLocation::LOCATION(cache));
//                // }
//                // Remote storages are fallback
//                for (auto remote_storage : remote_storage_services) {
//                    locations.insert(locations.end(), wrench::FileLocation::LOCATION(remote_storage));
//                }
//                file_locations[f] = locations;
//            }
//            // Identify output-file locations
//            for (auto f : task->getOutputFiles()) {
//                // Fill vector of output-file locations in order of write-priority
//                std::vector<std::shared_ptr<wrench::FileLocation>> locations;
//                // Write to first remote storage
//                //TODO: chose the output file location according to a logic?
//                for (auto remote_storage : remote_storage_services) {
//                    locations.insert(locations.end(), wrench::FileLocation::LOCATION(remote_storage));
//                }
//                file_locations[f] = locations;
//            }
//        }
//
//        // Create and submit a job for each chain
//        auto job = this->job_manager->createStandardJob(task_chunks, file_locations);
//        this->job_first_last_tasks[job] = first_last_tasks;
//        std::map<std::string, std::string> htcondor_service_specific_args = {};
//        //TODO: generalize to arbitrary numbers of htcondor services
//        this->job_manager->submitJob(job, htcondor_compute_service, htcondor_service_specific_args);
//    }
//
//    WRENCH_INFO("Done with scheduling tasks as standard jobs");


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
        WRENCH_INFO("Workload execution is complete!");
    } else{
        WRENCH_INFO("Workload execution is incomplete!")
    }

    WRENCH_INFO("SimpleExecutionController daemon started on host %s terminating", wrench::Simulation::getHostName().c_str());

    this->job_manager.reset();

    return 0;
}


/**
 * @brief Process a ExecutionEvent::COMPOUND_JOB_FAILURE
 * 
 * @param event: an execution event
 */
void SimpleExecutionController::processEventCompoundJobFailure(std::shared_ptr<wrench::CompoundJobFailedEvent> event) {
    WRENCH_INFO("Notified that compound job %s has failed!", event->job->getName().c_str());
    WRENCH_INFO("Failure case: %s", event->failure_cause->toString().c_str());
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

    /* Remove all tasks and compute incremental output values in one loop */
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
        start_date = std::min<double>(start_date, action->getStartDate());
        end_date = std::max<double>(end_date, action->getEndDate());
        if (action->getName().find("file_read_")) {
            incr_infile_transfertime += elapsed;
        } else if (action->getName().find("compute_")) {
            incr_compute_time += elapsed;
        } else if (action->getName().find("file_write_")) {
            incr_outfile_transfertime += elapsed;
        }
    }

    // Figure out file sizes
    for (auto const &f : this->workload_spec[event->job->getName()].infiles) {
        incr_infile_size += f->getSize();
    }
    incr_outfile_size += this->workload_spec[event->job->getName()].outfile->getSize();

//    /* Identify first/last tasks and harvest information*/
//    auto first_task = std::get<0>(this->job_first_last_tasks[job]);
//    auto last_task = std::get<1>(this->job_first_last_tasks[job]);
//    // get start and end date of job
//    double start_date = first_task->getReadInputStartDate();
//    double end_date = last_task->getWriteOutputEndDate();
//    // clear first/last tasks job records
//    this->job_first_last_tasks.erase(job);
//
//    /* Remove all tasks and compute incremental output values in one loop */
//    double incr_compute_time = 0.;
//    double incr_infile_transfertime = 0.;
//    double incr_infile_size = 0.;
//    double incr_outfile_transfertime = 0.;
//    double incr_outfile_size = 0.;
//    std::string execution_host = "";
//
//    for (auto const &task: job->getTasks()) {
//        // Compute job's time data
//        if (
//            (task->getComputationEndDate() != -1.0) && (task->getComputationStartDate() != -1.0)
//            && (task->getReadInputEndDate() != -1.0) && (task->getReadInputStartDate() != -1.0)
//            && (task->getWriteOutputEndDate() != -1.0) && (task->getWriteOutputStartDate() != -1.0)
//        ) {
//            incr_compute_time += (task->getComputationEndDate() - task->getComputationStartDate());
//            incr_infile_transfertime += (task->getReadInputEndDate() - task->getReadInputStartDate());
//            incr_outfile_transfertime += (task->getWriteOutputEndDate() - task->getWriteOutputStartDate());
//        }
//        else {
//            throw std::runtime_error("One of the task " + task->getID() + "'s date getters returned unmeaningful default value -1.0!");
//        }
//        // Compute job's I/O sizes
//        if (task->getInputFiles().size() > 0) {
//            for (auto infile : task->getInputFiles()) {
//                incr_infile_size += infile->getSize();
//            }
//        }
//        if (task->getOutputFiles().size() > 0) {
//            for (auto outfile : task->getOutputFiles()) {
//                incr_outfile_size += outfile->getSize();
//            }
//        }
//        if (execution_host == "") {
//            execution_host = task->getPhysicalExecutionHost();
//        }
//
//        // free some memory
//        this->getWorkflow()->removeTask(task);
//    }

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
