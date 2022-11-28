#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(cache_computation, "Log category for CacheComputation");

#include "CacheComputation.h"
#include "../MonitorAction.h"

//#define SIMULATE_FILE_LOOKUP_OPERATION 1


/**
 * @brief Construct a new CacheComputation::CacheComputation object
 * to be used as a lambda within a compute action, which shall take caching of input-files into account.
 * 
 * @param cache_storage_services Storage services reachable to retrieve and cache input files
 * @param grid_storage_services Storage services reachable to retrieve input files
 * @param files Input files of the job to process
 * @param total_flops Total #FLOPS of the whole compute action of the job
 */
CacheComputation::CacheComputation(std::set<std::shared_ptr<wrench::StorageService>> &cache_storage_services,
                                std::set<std::shared_ptr<wrench::StorageService>> &grid_storage_services,
                                std::vector<std::shared_ptr<wrench::DataFile>> &files,
                                double total_flops) {
    this->cache_storage_services = cache_storage_services;
    this->grid_storage_services = grid_storage_services;
    this->files = files;
    this->total_flops = total_flops;
    this->total_data_size = determineTotalDataSize(files);
}

/**
 * @brief Cache by the job required files on one of the local host's 
 * reachable cache storage services. 
 * Free space when needed according to an LRU scheme.
 * 
 * TODO: Find some optimal sources serving and destinations providing files to jobs.
 * TODO: Find solutions for possible race conditions, when several jobs require same files.
 * 
 * @param hostname Name of the host, where the job runs
 */
void CacheComputation::determineFileSourcesAndCache(std::shared_ptr<wrench::ActionExecutor> action_executor, bool cache_files = true) {

    std::string hostname = action_executor->getHostname(); // host where action is executed
    auto host = simgrid::s4u::Host::by_name(hostname); 
    std::string netzone = host->get_englobing_zone()->get_name(); // network zone executing host belongs to
    auto the_action = std::dynamic_pointer_cast<MonitorAction>(action_executor->getAction()); // executed action

    double cached_data_size = 0.;
    double remote_data_size = 0.;

    // Identify all cache storage services that can be reached from 
    // this host, which runs the streaming action
    std::vector<std::shared_ptr<wrench::StorageService>> matched_storage_services;

    for (auto const &ss : this->cache_storage_services) {
        bool host_in_scope = false;
        if (SimpleSimulator::local_cache_scope) {
            host_in_scope = (ss->getHostname() == hostname);
        } else {
            host_in_scope = (SimpleSimulator::hosts_in_zones[netzone].find(ss->getHostname()) != SimpleSimulator::hosts_in_zones[netzone].end());
        }
        if (host_in_scope) {
            matched_storage_services.push_back(ss);
            WRENCH_DEBUG("Found a reachable cache on host %s", ss->getHostname().c_str());
        }
    }
    if (matched_storage_services.empty()) {
        WRENCH_DEBUG("Couldn't find a reachable cache");
    }
    

    // For each file, identify where to read it from and/or deal with cache updates, etc.
    for (auto const &f : this->files) {
        // find a source providing the required file
        std::shared_ptr<wrench::StorageService> source_ss;
        // See whether the file is already available in a "reachable" cache storage service
        for (auto const &ss : matched_storage_services) {
#ifdef SIMULATE_FILE_LOOKUP_OPERATION
            bool has_file = ss->lookupFile(f, wrench::FileLocation::LOCATION(ss));
#else
            bool has_file = SimpleSimulator::global_file_map[ss].hasFile(f);
#endif
            if (has_file) {
                source_ss = ss;
                WRENCH_DEBUG("Found file %s with size %.2f in cache %s", f->getID().c_str(), f->getSize(), source_ss->getHostname().c_str());
                cached_data_size += f->getSize();
                break;
            }
        }
        // If yes, we're done
        if (source_ss) {
            SimpleSimulator::global_file_map[source_ss].touchFile(f.get());
            this->file_sources[f] = wrench::FileLocation::LOCATION(source_ss, f);
            continue;
        }
        // If not, then we have to copy the file from some GRID source to some reachable cache storage service
        // TODO: Find the optimal GRID source, whatever that means (right now it's whichever one works first)
        for (auto const &ss : this->grid_storage_services) {
#ifdef SIMULATE_FILE_LOOKUP_OPERATION
            bool has_file = ss->lookupFile(f, wrench::FileLocation::LOCATION(ss));
#else
            bool has_file = SimpleSimulator::global_file_map[ss].hasFile(f);
#endif
            if (has_file) {
                source_ss = ss;
                remote_data_size += f->getSize();
                break;
            }
        }
        if (!source_ss) {
            throw std::runtime_error("CacheComputation(): Couldn't find file " + f->getID() + " on any storage service!");
        } else {
            SimpleSimulator::global_file_map[source_ss].touchFile(f.get());
        }

        // When there is a reachable cache, cache the file and evict others when needed
        if (!matched_storage_services.empty()) {
            // Destination storage to cache the file
            // TODO: Find the optimal reachable cache destination, whatever that means (right now it's random, with a bad RNG!)
            auto destination_ss = matched_storage_services.at(rand() % matched_storage_services.size());

            // Evict files while to create space, using an LRU scheme!
            double free_space = destination_ss->getFreeSpace().begin()->second;
            while (free_space < f->getSize()) {
                auto to_evict = SimpleSimulator::global_file_map[destination_ss].removeLRUFile();
                WRENCH_INFO("Evicting file %s from storage service on host %s",
                            to_evict->getID().c_str(), destination_ss->getHostname().c_str());
                destination_ss->deleteFile(wrench::FileLocation::LOCATION(destination_ss, to_evict));
                free_space += to_evict->getSize();
            }

            // Instead of doing this file copy right here, instantly create the file locally for next jobs
            if (cache_files) {
                //? Alternative: Wait for computation to finish and copy file then
                // TODO: Better idea perhaps: have the first job that streams the file update a counter
                // TODO: of file blocks available at the storage service, and subsequent jobs
                // TODO: can read a block only if it's available (e.g., by waiting on some
                // TODO: condition variable, which is signaled by the first job each time it
                // TODO: reads a block).
                WRENCH_DEBUG("Caching file %s on storage %s", f->getID().c_str(), destination_ss->getHostname().c_str());
                // wrench::StorageService::copyFile(f, wrench::FileLocation::LOCATION(source_ss), wrench::FileLocation::LOCATION(destination_ss));
                wrench::Simulation::createFile(wrench::FileLocation::LOCATION(destination_ss, f));

                SimpleSimulator::global_file_map[destination_ss].touchFile(f.get());

                // this->file_sources[f] = wrench::FileLocation::LOCATION(destination_ss);
            }

        }

        this->file_sources[f] = wrench::FileLocation::LOCATION(source_ss, f);
    }

    // Fill monitoring information
    if (std::abs(cached_data_size + remote_data_size - this->total_data_size) > 1.) {
        throw std::runtime_error("There is more or less data read from cache plus remote than the job's input-data size!");
    }
    WRENCH_DEBUG("Hitrate: %.2f (Cached data: %.2f, total data: %.2f)", cached_data_size/this->total_data_size, cached_data_size, this->total_data_size);
    the_action->set_hitrate(cached_data_size/this->total_data_size);
}

//? Question for Henri: put this into determineFileSources function to prevent two times the same loop?
/**
 * @brief Determine the incremental size of all input-files of a job
 * 
 * @param files Input files of the job to consider
 * @return double
 */
double CacheComputation::determineTotalDataSize(const std::vector<std::shared_ptr<wrench::DataFile>> &files) {
    double incr_file_size = 0.0;
    for (auto const &f : this->files) {
        incr_file_size += f->getSize();
    }
    return incr_file_size;
}

/**
 * @brief Functor operator to be usable as lambda in custom action
 * 
 * @param action_executor 
 */
void CacheComputation::operator () (std::shared_ptr<wrench::ActionExecutor> action_executor) {
    std::string hostname = action_executor->getHostname();

    // Identify all file sources (and deal with caching, evictions, etc.
    WRENCH_INFO("Determining file sources for cache computation");
    this->determineFileSourcesAndCache(action_executor, SimpleSimulator::infile_caching_on);
    // Perform computation
    WRENCH_INFO("Performing the computation action");
    this->performComputation(action_executor);

}

/**
 * @brief Determine the share on the total number of FLOPS to be computed 
 * in the step processing a fraction of the full input data
 * 
 * @param data_size Size of the input-data block considered
 * @param total_data_size Total incremental size of all input-files
 * @return double 
 */
double CacheComputation::determineFlops(double data_size, double total_data_size) {
    double flops = this->total_flops * data_size / total_data_size;
    return flops;
}

/**
 * @brief Perform the computation within the simulation of the job
 * 
 * @param action_executor Handle to access the action this computation belongs to
 */
void CacheComputation::performComputation(std::shared_ptr<wrench::ActionExecutor> action_executor) {
    throw std::runtime_error(
        "Base class CacheComputation has no performComputation implemented! \
        It is meant only as a purely virtual placeholder. \
        Use one of the derived classes for the compute action!"
    );
}
