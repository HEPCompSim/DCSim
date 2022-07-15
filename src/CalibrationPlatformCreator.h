#ifndef DISTCACHESIM_CALIBRATIONPLATFORMCREATOR_H
#define DISTCACHESIM_CALIBRATIONPLATFORMCREATOR_H

#include <nlohmann/json.hpp>

class CalibrationPlatformCreator {

public:
    CalibrationPlatformCreator(nlohmann::json json_spec);

    void operator()() const {
        create_platform();
    }

private:

    void create_platform() const;


};


#endif //DISTCACHESIM_CALIBRATIONPLATFORMCREATOR_H
