/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef MY_SIMPLEWMS_H
#define MY_SIMPLEWMS_H

#include <wrench-dev.h>

class Simulation;

/**
 *  @brief A simple WMS implementation
 */
class SimpleWMS : public wrench::WMS {
public:
    // Constructor
    SimpleWMS(const std::set<std::shared_ptr<wrench::ComputeService>>& compute_services,
              const std::set<std::shared_ptr<wrench::StorageService>>& storage_services,
              const std::set<std::shared_ptr<wrench::NetworkProximityService>>& network_proximity_services,
              std::shared_ptr<wrench::FileRegistryService> file_registry_service,
              const std::string& hostname,
              const double& hitrate);

protected:
    void processEventStandardJobFailure(std::shared_ptr<wrench::StandardJobFailedEvent>) override;
    void processEventStandardJobCompletion(std::shared_ptr<wrench::StandardJobCompletedEvent>) override;

private:
    int main() override;

    /** @brief The job manager */
    std::shared_ptr<wrench::JobManager> job_manager;
    /** @brief The data movement manager */
    std::shared_ptr<wrench::DataMovementManager> data_movement_manager;
    /** @brief Whether the workflow execution shoulb be aborted */
    bool abort = false;
    /** @brief The desired fraction of input files served by the cache */
    double hitrate = 0.;


    std::map<std::shared_ptr<wrench::StandardJob>, std::pair<wrench::WorkflowTask*, wrench::WorkflowTask*>> job_first_last_tasks;


};

#endif //MY_SIMPLEWMS_H

