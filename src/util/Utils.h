

#ifndef S_UTILS_H
#define S_UTILS_H

#include <string>
#include <stdexcept>


/** 
 * @enum StorageServiceBufferType
 * @brief Types of wrench::StorageServiceProperty BUFFER_SIZE values  
 */
enum StorageServiceBufferType {
    Infinity, /* full buffering */
    Zero,     /* ideal (continous) flow model */
    Value     /* Any integral value between 0 and infinity corresponding to a real buffer size (small buffer size -> many simulation calls -> slower simulation) */
};

/**
 * @brief Get the StorageServiceProperty BUFFER_SIZE value type object
 * 
 * @param ssprop 
 * @return StorageServiceBufferType 
 */
inline StorageServiceBufferType get_ssbuffer_type(const std::string &ssprop) {
    if ((ssprop == "infinity") or (ssprop == "inf")) {
        return StorageServiceBufferType::Infinity;
    } else if ((ssprop == "0") or (ssprop == "zero")) {
        return StorageServiceBufferType::Zero;
    } else {
        if ((!ssprop.empty()) && (ssprop.find_first_not_of("0123456789") == std::string::npos) && (std::stoll(ssprop) > 0)) {
            return StorageServiceBufferType::Value;
        } else {
            throw std::runtime_error("StorageService buffer value " + ssprop + "invalid. Please choose 'infinity', 'zero' or a positive long integer value in between");
        }
    }
}


#endif//S_UTILS_H
