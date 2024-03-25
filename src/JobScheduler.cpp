

#include "JobScheduler.h"
#include "WorkloadExecutionController.h"


JobScheduler::JobScheduler(const std::vector<std::shared_ptr<wrench::ComputeService>> &compute_services)  {
    this->compute_services = compute_services;
    this->total_num_idle_cores = 0;

    // Create view of available resources
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

void JobScheduler::addExecutionController(WorkloadExecutionController *execution_controller) {
    this->execution_controllers.push_back(execution_controller);
}


void JobScheduler::jobDone(const std::shared_ptr<wrench::CompoundJob> &job) {
    // Num cores
    std::get<0>(this->available_resources[job->getParentComputeService()])++;
    // RAM
    // TODO: Check that the RAM is what we think it is
    std::get<1>(this->available_resources[job->getParentComputeService()]) += job->getMinimumRequiredMemory();
    this->total_num_idle_cores += job->getMinimumRequiredNumCores();
}

void JobScheduler::schedule() {

    if (this->total_num_idle_cores == 0) {
        return;
    }

    // Go through the workload execution controllers in order
    for (auto const &ec: this->execution_controllers) {
        if (ec->isWorkloadEmpty()) {
            continue; // all jobs have been submitted fo this execution controller
        }
        std::cerr << "TRYING TO SCHEDULE A JOB FROM EC " << ec->getName() << "\n";

        // Loop through all the jobs in the workload in sequence
        for (const auto &job_spec: ec->get_workload_spec()) {
            auto job_name = job_spec.first;
            auto num_cores = job_spec.second.cores;
            auto total_ram = job_spec.second.total_mem;

            if (this->total_num_idle_cores == 0) {
                return;
            }

            std::cerr << "TRYING TO SCHEDULE: " << job_name << " " << num_cores << " " << total_ram << "\n";

            for (auto const &entry: this->available_resources) {
                std::cerr << "Looking at CS: " << entry.first->getName() << "\n";
                if ((num_cores <= std::get<0>(entry.second)) and
                    (total_ram <= std::get<1>(entry.second))) {
                    // Create the job and schedule it
                    std::cerr << "YES!!\n";
                    auto job = ec->createAndSubmitJob(job_name, entry.first);
                    std::cerr << "JOB CREATED AND SUBMITTED\n";
                    std::get<0>(this->available_resources[entry.first]) -= num_cores;
                    std::get<1>(this->available_resources[entry.first]) -= total_ram;
                    this->total_num_idle_cores -= num_cores;
                    break;
                }
            }
        }
    }
}