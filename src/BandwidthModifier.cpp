/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <iostream>
#include <algorithm>

#include "BandwidthModifier.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(bandwidth_modifier, "Log category for BandwidthModifier");

/**
 *  @brief A "controller" that dynamically modifies a link's bandwidth
 * 
 *  @param hostname: the host on which this actor runs
 *  @param link_name: the name of the link whose bandwidth is to be modified
 *  @param period: the period in between two bandwidth modifications (in seconds)
 *  @param distribution: the normal distribution from which to same bandwidth modification values
 */
BandwidthModifier::BandwidthModifier(
        const std::string &hostname,
        const std::string &link_name,
        double period,
        std::normal_distribution<double>* distribution,
        unsigned long seed) : wrench::ExecutionController(
        hostname,
        "bandwidth-modifier"), link_name(link_name), period(period), distribution(distribution), seed(seed) {

}

/**
 * @brief main method of the BandwidthModifier actor
 * 
 * @return 0 on completion
 * 
 * @throw std::runtime_error
 */
int BandwidthModifier::main() {

    wrench::TerminalOutput::setThisProcessLoggingColor(wrench::TerminalOutput::COLOR_GREEN);

    // Retrieve the link
    auto the_link = simgrid::s4u::Link::by_name_or_null(this->link_name);
    if (the_link == nullptr) {
        throw std::invalid_argument("BandwidthModifier::main(): The platform does not contain a link named '" + this->link_name +"'");
    }

    // Get its (original) bandwidth
    const double original_bandwidth = the_link->get_bandwidth();

    // Create RNG
    std::mt19937 rng(this->seed);

    while (true) {
        // Sample bandwidth modification subtrahend
        double reduction = (*this->distribution)(rng);
        while ((original_bandwidth - reduction < 0) || reduction < 0) reduction = (*this->distribution)(rng);
        // Update the link bandwidth
        the_link->set_bandwidth(original_bandwidth - reduction);
        // Sleep during the period
        wrench::Simulation::sleep(period);
    }
}

