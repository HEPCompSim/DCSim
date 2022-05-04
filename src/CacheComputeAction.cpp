#include "CacheComputeAction.h"

/**
 * @brief Constructor that adds some more parameters for monitoring purposes
 */
CacheComputeAction::CacheComputeAction(
    const std::string &name, std::shared_ptr<wrench::CompoundJob> job,
    double ram,
    unsigned long num_cores,
    const std::function<void(std::shared_ptr<wrench::ActionExecutor> action_executor)> &lambda_execute,
    const std::function<void(std::shared_ptr<wrench::ActionExecutor> action_executor)> &lambda_terminate,
    double in_transfer_time = -1.,
    double cpu_time = -1.,
    // double out_transfer_time = -1.,
    double hitrate = -1.
) : CustomAction(name, ram, num_cores, std::move(lambda_execute), std::move(lambda_terminate)),
infile_transfer_time(in_transfer_time),
calculation_time(cpu_time),
// outfile_transfer_time(out_transfer_time),
hitrate(hitrate) {}
