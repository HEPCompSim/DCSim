

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

    std::cerr << "I AM IN SCHEDULE!\n";

    // Pick a workload in an execution controller
    std::shared_ptr<WorkloadExecutionController> picked_execution_controller = nullptr;
    for (auto const &ec: this->execution_controllers) {
        if (ec->isWorkloadEmpty()) {
            continue;
        }

        // Loop through all the jobs in the workload until
        for (const auto &job_spec: ec->get_workload_spec()) {
            auto job_name = job_spec.first;
            auto num_cores = job_spec.second.cores;
            auto total_ram = job_spec.second.total_mem;

            // TODO: Do this efficiently via priority queues/maps etc,
            //  stop when for sure nothing can be scheduled etc
            // Pick a compute services
            for (auto const &entry: this->available_resources) {
                if ((num_cores <= std::get<0>(entry.second)) and
                    (total_ram <= std::get<1>(entry.second))) {
                    // Create the job and schedule it
                    auto job = ec->createAndSubmitJob(job_name, entry.first);
                    std::get<0>(this->available_resources[entry.first]) -= num_cores;
                    std::get<1>(this->available_resources[entry.first]) -= total_ram;
                    this->total_num_idle_cores -= num_cores;
                }
            }
        }
    }
}