

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


/** 
 * @enum StorageServiceBufferType
 * @brief Types of wrench::StorageServiceProperty BUFFER_SIZE values  
 */
enum StorageServiceBufferType {
    Infinity, /* full buffering */
    Zero, /* ideal (continous) flow model, not implemented yet */
    Value /* Amakny integral value between 0 and infinity corresponding to a real buffer size (small buffer size -> many simulation calls -> slower simulation) */
};

/**
 * @brief Get the StorageServiceProperty BUFFER_SIZE value type object
 * 
 * @param ssprop 
 * @return StorageServiceBufferType 
 */
inline StorageServiceBufferType get_ssbuffer_type(std::string ssprop) {
    if(ssprop == "infinity") {
        return StorageServiceBufferType::Infinity;
    }
    else if ((ssprop == "0") or (ssprop == "zero")) {
         return StorageServiceBufferType::Zero;
    }
    else {
        if ((!ssprop.empty()) && (ssprop.find_first_not_of("0123456789")==std::string::npos) && (std::stoll(ssprop) > 0)) {
            return StorageServiceBufferType::Value;
        }
        else {
            throw std::runtime_error("StorageService buffer value " + ssprop + "invalid. Please choose 'infinity', 'zero' or a positive long integer value in between");
        }
    }
}


#endif //S_ENUMS_H

