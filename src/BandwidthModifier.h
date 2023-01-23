/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef MY_BANDWIDTH_MODIFIER_H
#define MY_BANDWIDTH_MODIFIER_H

#include <wrench-dev.h>
#include <iostream>
#include <fstream>

#include "JobSpecification.h"
#include "LRU_FileList.h"

#include "util/Enums.h"

class Simulation;

class BandwidthModifier : public wrench::ExecutionController {
public:
    // Constructor
    BandwidthModifier(
              const std::string &hostname,
              const std::string &link_name,
              double period,
              std::normal_distribution<double>* distribution,
              unsigned long seed);

private:

    std::string link_name;
    double period;
    std::normal_distribution<double>* distribution;
    std::mt19937 generator;
    unsigned long seed;

    int main() override;

};

#endif //MY_BANDWIDTH_MODIFIER_H

