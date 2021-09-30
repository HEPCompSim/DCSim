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

/**
 * @brief helper function wich checks if a string ends with a desired suffix
 * 
 * @param str: string to check
 * @param suffix: suffix to match to
 */
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
    exit (EXIT_FAILURE);
  } catch (const std::out_of_range& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Number out of range: " << arg << std::endl;
    exit (EXIT_FAILURE);
  }
}

/**
 * @brief helper function for converting CLI argument to size_t
 *
 * @param arg: a CLI argument string
 */
size_t arg_to_sizet (const std::string& arg) {
  try {
    std::size_t pos;
    size_t value = std::stoi(arg, &pos);
    if (pos < arg.size()) {
      std::cerr << "Trailing characters after number: " << arg << std::endl;
    }
    return value;
  } catch (const std::invalid_argument& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Invalid number: " << arg << std::endl;
    exit (EXIT_FAILURE);
  } catch (const std::out_of_range& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Number out of range: " << arg << std::endl;
    exit (EXIT_FAILURE);
  }
}


int main(int argc, char **argv) {

  // Declaration of the top-level WRENCH simulation object
  auto simulation = new wrench::Simulation();

  // Initialization of the simulation
  simulation->init(&argc, argv);

  // Parsing of the command-line arguments for this WRENCH simulation
  if (argc != 6) {
    std::cerr << "Usage: " << argv[0];
    std::cerr << " <xml platform file> <number of jobs> <input files per job> <average inputfile size> <cache hitrate>";
    std::cerr << " [--wrench-full-log || --log=custom_wms.threshold=info]";
    std::cerr << std::endl;
    exit(1);
  }

  // The first argument is the platform description file, written in XML following the SimGrid-defined DTD
  char *platform_file = argv[1];
  // The second argument is the number of jobs which need to be executed
  size_t num_jobs = arg_to_sizet(argv[2]);
  // The third argument is the number of input files per job which need to be transferred
  size_t infiles_per_job = arg_to_sizet(argv[3]);
  // The fourth argument is the average size of the inputfiles in bytes
  double average_infile_size = arg_to_double(argv[4]);
  // The fifth argument is the fractional cache hitrate
  double hitrate = arg_to_double(argv[5]);

  // Create a workflow
  std::cerr << "Loading workflow..." << std::endl;
  auto workflow = new wrench::Workflow();

  // Sample task parameters from normal distributions
  // Set normal distribution parameters
  double average_flops = 1200000;
  double average_memory = 2000000000;
  double sigma_flops = 0.5*average_flops;
  double sigma_memory = 0.5*average_memory;
  double sigma_infile_size = 0.5*average_infile_size;

  // Initialize random number generators
  std::mt19937 gen(42);
  std::normal_distribution<> flops(average_flops, sigma_flops);
  std::normal_distribution<> mem(average_memory, sigma_memory);
  std::normal_distribution<> insize(average_infile_size, sigma_infile_size);

  for (size_t j = 0; j < num_jobs; j++) {
    // Sample task parameter values
    double dflops = flops(gen);
    // and resample when negative
    while (dflops < 0.) dflops = flops(gen);
    double dmem = mem(gen);
    while (dmem < 0.) dmem = mem(gen);
    auto task = workflow->addTask("task_"+std::to_string(j), dflops, 1, 1, dmem);
    for (size_t f = 0; f < infiles_per_job; f++) {
      double dinsize = insize(gen);
      while (dinsize < 0.) dinsize = insize(gen); 
      task->addInputFile(workflow->addFile("infile_"+std::to_string(j)+"_"+std::to_string(f), dinsize));
    }
    task->addOutputFile(workflow->addFile("outfile_"+std::to_string(j), 0.0));
  }
  std::cerr << "The workflow has " << workflow->getNumberOfTasks() << " tasks " << std::endl;

  // Reading and parsing the platform description file to instantiate a simulated platform
  std::cerr << "Instantiating SimGrid platform..." << std::endl;
  simulation->instantiatePlatform(platform_file);

  // Loop over vector of all the hosts in the simulated platform
  std::vector<std::string> hostname_list = simulation->getHostnameList();
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
      auto storage_service = simulation->add(new wrench::SimpleStorageService(storage_host, {"/"}));
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
      condor_compute_resources.insert(simulation->add(new wrench::BareMetalComputeService(
        *hostname,
        {std::make_pair(
          *hostname,
          std::make_tuple(
            wrench::Simulation::getHostNumCores(*hostname),
            wrench::Simulation::getHostMemoryCapacity(*hostname)
          )
        )},
        "/"
      )));
    }
  }
  // Instantiate a HTcondorComputeService and add it to the simulation
  std::set<shared_ptr<wrench::ComputeService>> htcondor_compute_services;
  htcondor_compute_services.insert(shared_ptr<wrench::ComputeService>(simulation->add(
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
  )));


  // Instantiate a file registry service
  std::string file_registry_service_host = wms_host;
  std::cerr << "Instantiating a FileRegistryService on " << file_registry_service_host << "..." << std::endl;
  auto file_registry_service =
          simulation->add(new wrench::FileRegistryService(file_registry_service_host));

  // Instantiate a network proximity service
  std::string network_proximity_service_host = wms_host;
  std::cerr << "Instantiating a NetworkProximityService on " << network_proximity_service_host << "..." << std::endl;
  auto network_proximity_service =
          simulation->add(new wrench::NetworkProximityService(
            network_proximity_service_host, 
            hostname_list, 
            {}, 
            {}
          ));


  // Instantiate a WMS
  auto wms = simulation->add(
          new SimpleWMS(
            htcondor_compute_services, 
            storage_services,
            {},//{network_proximity_service},
            nullptr,//file_registry_service, 
            wms_host,
            hitrate
          )
  );
  wms->addWorkflow(workflow);


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
         simulation->stageFile(f, remote_storage_service);
     }
  } catch (std::runtime_error &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 0;
  }

  // Launch the simulation
  std::cerr << "Launching the Simulation..." << std::endl;
  try {
    simulation->launch();
  } catch (std::runtime_error &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 0;
  }
  std::cerr << "Simulation done!" << std::endl;

  // Analyse event traces
  auto simulation_output = simulation->getOutput();
  auto trace = simulation_output.getTrace<wrench::SimulationTimestampTaskCompletion>();
  for (auto const &item : trace) {
    std::cerr << "Task " << item->getContent()->getTask()->getID() << " completed at time " << item->getDate() << std::endl;
  }
  // and dump JSONs containing the generated data
  std::cerr << "Dumping generated data..." << std::endl;
  simulation_output.dumpDiskOperationsJSON("tmp/diskOps.json", true);
  simulation_output.dumpLinkUsageJSON("tmp/linkUsage.json", true);
  simulation_output.dumpPlatformGraphJSON("tmp/platformGraph.json", true);
  simulation_output.dumpWorkflowExecutionJSON(workflow, "tmp/workflowExecution.json", false, true);

  return 0;
}

