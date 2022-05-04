

#ifndef MY_CACHE_COMPUTE_ACTION_H
#define MY_CACHE_COMPUTE_ACTION_H

#include <wrench-dev.h>

/**
 * @brief Extension of CustomAction to monitor job execution
 */
class MonitorAction : public wrench::CustomAction {
public:
    /**
     * @brief Constructor that adds some more parameters for monitoring purposes
     */
    MonitorAction(
        const std::string &name,
        double ram,
        unsigned long num_cores,
        const std::function<void(std::shared_ptr<wrench::ActionExecutor> action_executor)> &lambda_execute,
        const std::function<void(std::shared_ptr<wrench::ActionExecutor> action_executor)> &lambda_terminate
    );

    double get_infile_transfer_time() {
        return infile_transfer_time;
    }
    double get_calculation_time() {
        return calculation_time;
    }
    // double get_outfile_transfer_time() {
    //     return outfile_transfer_time;
    // }
    double get_hitrate() {
        return hitrate;
    }

    void set_infile_transfer_time(double value) {
        this->infile_transfer_time = value;
    }
    void set_calculation_time(double value) {
        this->calculation_time = value;
    }
    // void set_outfile_transfer_time(double value) {
    //     this->outfile_transfer_time = value;
    // }
    void set_hitrate(double value) {
        this->hitrate = value;
    }
    
protected:
    /** @brief Attribute monitoring accumulated transfer-time of input-files.
     * Non-zero for jobs where infile-read and compute steps are separated. */
    double infile_transfer_time; 
    /** @brief Attribute monitoring the accumulated computation time (CPU time).*/
    double calculation_time;
    // /** @brief Atrribute monitoring accumulated transfer-time of output files.
    //  * Currently not in use. */
    // double outfile_transfer_time; // transfer time for output files
    /** @brief Attribute monitoring fraction of input-files read from cache.
     * This might be dependent on the cache definition. */
    double hitrate;

};

#endif //MY_SIMPLE_EXECUTION_CONTROLLER_H