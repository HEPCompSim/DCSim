/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <wrench.h>
#include "SimpleWMS.h"

static bool ends_with(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

/**
 * @brief helper function for converting a CLI argument string to double
 * 
 * @param arg: a CLI argument string
 */
double arg_to_double (const std::string& arg) {
  try {
    std::size_t pos;
    double value = std::stoi(arg, &pos);
    if (pos < arg.size()) {
      std::cerr << "Trailing characters after number: " << arg << std::endl;
    }
    return value;
  } catch (const std::invalid_argument& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Invalid number: " << arg << std::endl;
  } catch (const std::out_of_range& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Number out of range: " << arg << std::endl;
  }
}

int main(int argc, char **argv) {

  // Declaration of the top-level WRENCH simulation object
  wrench::Simulation simulation;

  // Initialization of the simulation
  simulation.init(&argc, argv);

  // Parsing of the command-line arguments for this WRENCH simulation
  if (argc != 6) {
    std::cerr << "Usage: " << argv[0];
    std::cerr << " <xml platform file> <number of jobs> <input files per job> <average inputfile size> <cache hitrate>";
    std::cerr << std::endl;
    exit(1);
  }

  // The first argument is the platform description file, written in XML following the SimGrid-defined DTD
  char *platform_file = argv[1];
  // The second argument is the number of jobs which need to be executed
  std::string tmp_arg = argv[2];
  int num_jobs;
  try
  {
    std::size_t pos;
    num_jobs = std::stoi(tmp_arg, &pos);
    if (pos < tmp_arg.size()) {
      std::cerr << "Trailing characters after number: " << tmp_arg << std::endl;
    }
  } catch (const std::invalid_argument& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Invalid number: " << tmp_arg << std::endl;
  } catch (const std::out_of_range& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Number out of range: " << tmp_arg << std::endl;
  }
  // The third argument is the number of input files per job which need to be transferred
  tmp_arg = argv[3];
  int infiles_per_job;
  try
  {
    std::size_t pos;
    infiles_per_job = std::stoi(tmp_arg, &pos);
    if (pos < tmp_arg.size()) {
      std::cerr << "Trailing characters after number: " << tmp_arg << std::endl;
    }
  } catch (const std::invalid_argument& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Invalid number: " << tmp_arg << std::endl;
  } catch (const std::out_of_range& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Number out of range: " << tmp_arg << std::endl;
  }
  // The fourth argument is the average size of the inputfiles in bytes
  tmp_arg = argv[4];
  double average_infile_size;
  try
  {
    std::size_t pos;
    average_infile_size = std::stod(tmp_arg, &pos);
    if (pos < tmp_arg.size()) {
      std::cerr << "Trailing characters after number: " << tmp_arg << std::endl;
    }
  } catch (const std::invalid_argument& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Invalid number: " << tmp_arg << std::endl;
  } catch (const std::out_of_range& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Number out of range: " << tmp_arg << std::endl;
  }
  // The fifth argument is the fractional cache hitrate
  tmp_arg = argv[5];
  double hitrate;
  try
  {
    std::size_t pos;
    hitrate = std::stod(tmp_arg, &pos);
    if (pos < tmp_arg.size()) {
      std::cerr << "Trailing characters after number: " << tmp_arg << std::endl;
    }
  } catch (const std::invalid_argument& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Invalid number: " << tmp_arg << std::endl;
  } catch (const std::out_of_range& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Number out of range: " << tmp_arg << std::endl;
  }
  


  // Reading and parsing the workflow description file to create a wrench::Workflow object
  std::cerr << "Loading workflow..." << std::endl;
  wrench::Workflow* workflow;

  double average_flops = 1200000;
  double average_memory = 2000000000;
  for (size_t j = 0; j < num_jobs; j++) {
    auto task = workflow->addTask("task_"+j, average_flops, 1, 1, average_memory);
    for (size_t f = 0; f < infiles_per_job; f++) {
      task->addInputFile(workflow->addFile("infile_"+std::to_string(j)+"_"+std::to_string(f), average_infile_size));
    }
    task->addOutputFile(workflow->addFile("outfile_"+std::to_string(j), 0.0));
  }
  std::cerr << "The workflow has " << workflow->getNumberOfTasks() << " tasks " << std::endl;

  // Reading and parsing the platform description file to instantiate a simulated platform
  std::cerr << "Instantiating SimGrid platform..." << std::endl;
  simulation.instantiatePlatform(platform_file);

  // Loop over vector of all the hosts in the simulated platform
  std::vector<std::string> hostname_list = simulation.getHostnameList();
  // Create a list of storage services that will be used by the WMS
  std::set<std::shared_ptr<wrench::StorageService>> storage_services;
  // Split into cache storages
  std::set<std::shared_ptr<wrench::StorageService>> cache_storage_services;
  // and a remote storage that is able to serve all file requests
  std::set<std::shared_ptr<wrench::StorageService>> remote_storage_services;
  // Create a list of compute services that will be used by the HTCondorService
  std::set<std::shared_ptr<wrench::ComputeService>> condor_compute_resources;
  std::string wms_host = "WMSHost";
  for (std::vector<std::string>::iterator hostname = hostname_list.begin(); hostname != hostname_list.end(); ++hostname)
  {
    std::string hostname_transformed = *hostname;
    std::for_each(hostname_transformed.begin(), hostname_transformed.end(), [](char& c){c = std::tolower(c);});
    // Instantiate storage services
    // WMSHost doesn't need a StorageService
    if (*hostname != wms_host) {
      std::string storage_host = *hostname;
      std::cerr << "Instantiating a SimpleStorageService on " << storage_host << "..." << std::endl;
      auto storage_service = simulation.add(new wrench::SimpleStorageService(storage_host, {"/"}));
      if (hostname_transformed.find("remote") != std::string::npos) {
        remote_storage_services.insert(storage_service);
      } else {
        cache_storage_services.insert(storage_service);
      }
      storage_services.insert(storage_service);
    } 
    // Instantiate bare-metal compute-services
    if (
      (*hostname != wms_host) && 
      (hostname_transformed.find("storage") == std::string::npos)
    ) {
      condor_compute_resources.insert(new wrench::BareMetalComputeService(
        *hostname,
        {std::make_pair(
          *hostname,
          std::make_tuple(
            wrench::Simulation::getHostNumCores(*hostname),
            wrench::Simulation::getHostMemoryCapacity(*hostname)
          )
        )},
        "/"
      ));
    }
  }
  // Instantiate a HTcondorComputeService and add it to the simulation
  auto htcondor_compute_service = simulation.add(
    new wrench::HTCondorComputeService(
      wms_host,
     condor_compute_resources,
      {
        {wrench::HTCondorComputeServiceProperty::NEGOTIATOR_OVERHEAD, "1.0"},
        {wrench::HTCondorComputeServiceProperty::GRID_PRE_EXECUTION_DELAY, "10.0"},
        {wrench::HTCondorComputeServiceProperty::GRID_POST_EXECUTION_DELAY, "10.0"},
        {wrench::HTCondorComputeServiceProperty::NON_GRID_PRE_EXECUTION_DELAY, "5.0"},
        {wrench::HTCondorComputeServiceProperty::NON_GRID_POST_EXECUTION_DELAY, "5.0"}
      },
      {}
    )
  );


  // Instantiate a WMS
  auto wms = simulation.add(
          new SimpleWMS(
            htcondor_compute_service, 
            storage_services, 
            wms_host,
            hitrate
          )
  );
  wms->addWorkflow(workflow);

  // Instantiate a file registry service
  std::string file_registry_service_host = wms_host;
  std::cerr << "Instantiating a FileRegistryService on " << file_registry_service_host << "..." << std::endl;
  auto file_registry_service =
          new wrench::FileRegistryService(file_registry_service_host);
  simulation.add(file_registry_service);

  // // Instantiate a network proximity service
  // std::string network_proximity_service_host = wms_host;
  // std::cerr << "Instantiating a NetworkProximityService on " << network_proximity_service_host << "..." << std::endl;
  // auto network_proximity_service =
  //         new wrench::NetworkProximityService(
  //           network_proximity_service_host, 
  //           hostname_list, 
  //           {}, 
  //           {}
  //         );
  // simulation.add(network_proximity_service);

  // Check that the right remote_storage_service is passed for initial inputfile storage
  if (remote_storage_services.size() != 1) {
    throw std::runtime_error("This example Simple Simulator requires a single remote_storage_service");
  }
  auto remote_storage_service = *remote_storage_services.begin();

  // It is necessary to store, or "stage", input files
  std::cerr << "Staging input files..." << std::endl;
  auto input_files = workflow->getInputFiles();
  try {
     for (auto const &f : input_files) {
         simulation.stageFile(f, remote_storage_service);
     }
  } catch (std::runtime_error &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 0;
  }

  // Launch the simulation
  std::cerr << "Launching the Simulation..." << std::endl;
  try {
    simulation.launch();
  } catch (std::runtime_error &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 0;
  }
  std::cerr << "Simulation done!" << std::endl;

  // Analyse event traces
  auto simulation_output = simulation.getOutput();
  auto trace = simulation_output.getTrace<wrench::SimulationTimestampTaskCompletion>();
  for (auto const &item : trace) {
    std::cerr << "Task " << item->getContent()->getTask()->getID() << " completed at time " << item->getDate() << std::endl;
  }
  // and dump JSONs containing the generated data
  std::cerr << "Dumping generated data..." << std::endl;
  simulation_output.dumpDiskOperationsJSON("diskOps.json", true);
  simulation_output.dumpLinkUsageJSON("linkUsage.json", true);
  simulation_output.dumpPlatformGraphJSON("platformGraph.json", true);
  simulation_output.dumpWorkflowExecutionJSON(workflow, "workflowExecution.json", false, true);

  return 0;
}

