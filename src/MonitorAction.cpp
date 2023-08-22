#include "MonitorAction.h"

/**
 * @brief Constructor that adds some more parameters for monitoring purposes
 */
MonitorAction::MonitorAction(
        const std::string &name,
        double ram,
        unsigned long num_cores,
        const std::function<void(std::shared_ptr<wrench::ActionExecutor> action_executor)> &lambda_execute,
        const std::function<void(std::shared_ptr<wrench::ActionExecutor> action_executor)> &lambda_terminate) : CustomAction(name, ram, num_cores,
                                                                                                                             std::move(lambda_execute),
                                                                                                                             std::move(lambda_terminate)) {
    this->calculation_time = DefaultValues::UndefinedDouble;
    this->infile_transfer_time = DefaultValues::UndefinedDouble;
    // this->outfile_transfer_time = 0.;
    this->hitrate = DefaultValues::UndefinedDouble;
}
