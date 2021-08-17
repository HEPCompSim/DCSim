/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef MY_SIMPLESCHEDULER_H
#define MY_SIMPLESCHEDULER_H

#include <wrench-dev.h>

class SimpleHTCondorJobScheduler : public wrench::StandardJobScheduler {
public:
  SimpleHTCondorJobScheduler(
    std::set<std::shared_ptr<wrench::StorageService>> worker_storage_services, 
    std::set<std::shared_ptr<wrench::StorageService>> remote_storage_services
  )
    : worker_storage_services(worker_storage_services)
    , remote_storage_services(remote_storage_services) {}

  void scheduleTasks(const std::set<std::shared_ptr<wrench::HTCondorComputeService>> &compute_services,
                     const std::vector<wrench::WorkflowTask *> &tasks,
                     const double &hitrate);

private:
  std::set<std::shared_ptr<wrench::StorageService>> worker_storage_services;
  std::set<std::shared_ptr<wrench::StorageService>> remote_storage_services;
};

#endif //MY_SIMPLESCHEDULER_H

