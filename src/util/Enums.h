

#ifndef S_ENUMS_H
#define S_ENUMS_H

#include <string>
#include <stdexcept>

/**
 * @brief Set of enums that can be used in the simulation and related methods.
 */

enum WorkflowType {Calculation, Streaming, Copy};

inline WorkflowType get_workflow_type(std::string wfname) {
    if(wfname == "calculation") {
        return WorkflowType::Calculation;
    }
    else if (wfname == "streaming") {
        return WorkflowType::Streaming;
    }
    else if (wfname == "copy") {
        return WorkflowType::Copy;
    }
    else {
        throw std::runtime_error("Workflow type " + wfname + " invalid. Please choose 'calculation', 'streaming', or 'copy'");
    }
}


#endif //S_ENUMS_H