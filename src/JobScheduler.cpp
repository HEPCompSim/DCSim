

#include "JobScheduler.h"
#include "WorkloadExecutionController.h"

/**
 * @brief Constructor
 * @param compute_services: the set of compute services that this job scheduler will submit jobs to
 */
JobScheduler::JobScheduler(const std::vector<std::shared_ptr<wrench::ComputeService>> &compute_services)  {
    this->total_num_idle_cores = 0;

    // Create view of available resources as a map of compute services
    // to core and ram availability
    for (auto const &cs : compute_services) {
        if (cs->getNumHosts() != 1) {
            throw std::invalid_argument("JobScheduler::init(): Only 1-host compute services are supported!");
        }
        unsigned long num_cores = cs->getPerHostNumCores().begin()->second;
        double ram = cs->getMemoryCapacity().begin()->second;
        this->total_num_idle_cores += num_cores;

        this->available_resources[cs] = std::tuple(num_cores, ram);
    }
}

/**
 * @brief A a new execution controller that has a workload that this job scheduler should handle
 * @param execution_controller
 */
void JobScheduler::addExecutionController(WorkloadExecutionController *execution_controller) {
    this->execution_controllers.push_back(execution_controller);
}

/**
 * @brief A method to notify the scheduler that a job has completed
 * @param job
 */
void JobScheduler::jobDone(const std::shared_ptr<wrench::CompoundJob> &job) {
    // Num cores
    std::get<0>(this->available_resources[job->getParentComputeService()])++;
    // RAM   TODO: Check that the RAM is what we think it is
    std::get<1>(this->available_resources[job->getParentComputeService()]) += job->getMinimumRequiredMemory();
    this->total_num_idle_cores += job->getMinimumRequiredNumCores();
}

/**
 * @brief A method that should be invoked whenever there may be schedulable jobs
 */
void JobScheduler::schedule() {

    if (this->total_num_idle_cores == 0) {
        return;
    }

    // Go through the workload execution controllers in order
    for (auto const &ec: this->execution_controllers) {
        if (ec->isWorkloadEmpty()) {
            continue; // all jobs have been submitted fo this execution controller
        }

        // Loop through all the jobs in the workload in sequence
        std::vector<std::string> scheduled_jobs;
        for (const auto &job_spec: ec->get_workload_spec()) {
            auto job_name = job_spec.first;
            auto num_cores = job_spec.second.cores;
            auto total_ram = job_spec.second.total_mem;

            if (this->total_num_idle_cores == 0) {
                return;
            }

            // See if there is an compute service that can accommodate the job
            auto target_cs = pickComputeService(num_cores, total_ram);
            if (target_cs) {
                ec->createAndSubmitJob(job_name, target_cs);
                std::get<0>(this->available_resources[target_cs]) -= num_cores;
                std::get<1>(this->available_resources[target_cs]) -= total_ram;
                this->total_num_idle_cores -= num_cores;
                scheduled_jobs.push_back(job_name);
            }
        }

        // Clear jobs from the workload
        for (auto const &job_name : scheduled_jobs) {
            ec->setJobSubmitted(job_name);
        }
    }
}

/**
 * @brief Find a compute service with sufficient available resources
 * @param num_cores: the needed number of cores
 * @param total_ram: the needed RAM footprint
 * @return a compute service
 */
std::shared_ptr<wrench::ComputeService> JobScheduler::pickComputeService(unsigned long num_cores, double total_ram) {
    // Just a linear search right now, picking the first
    // compute service that works
    for (auto const &entry: this->available_resources) {
        if ((num_cores <= std::get<0>(entry.second)) and
            (total_ram <= std::get<1>(entry.second))) {
            return entry.first;
        }
    }
    return nullptr;
}