#include "CalibrationPlatformCreator.h"
#include <wrench-dev.h>

// This JSON object encodes default values, which may be overridden
// by user-specified values
nlohmann::json calibration_values =
        {
                {"sg01_flops", "1969.583Mf"},
                {"sg03_flops", "1971.000Mf"},
                {"sg04_flops", "1966.674Mf"},
                {"ssd_cache1_disk_read_bw", "0.75GBps"},
                {"ssd_cache1_disk_write_bw", "0.75GBps"},
                {"ssd_cache3_disk_read_bw", "0.75GBps"},
                {"ssd_cache3_disk_write_bw", "0.75GBps"},
                {"ssd_cache4_disk_read_bw", "0.75GBps"},
                {"ssd_cache4_disk_write_bw", "0.75GBps"},
                {"etp_link_to_switch_bandwidth", "10GBps"},
                {"etp_linkOut_bandwidth", "10GBps"},
                {"remote_storage_disk_read_bw", "40GBps"},
                {"remote_storage_disk_write_bw", "40GBps"},
                {"etp_to_remote_link_network_bandwidth", "10GBps"},
        };


/**
 * @brief Constructor that takes in a user-provided JSON specification that overrides
 *        default calibration parameter values
 * @param json_spec calibration platform spec in JSON
 */
CalibrationPlatformCreator::CalibrationPlatformCreator(nlohmann::json json_spec) {

    for (auto it = calibration_values.begin(); it != calibration_values.end(); ++it) {
        auto key = it.key();
        auto value = it.value();
        if (json_spec.find(key) != json_spec.end()) {
            calibration_values[key] = json_spec[key];
        }
    }
    std::cerr << calibration_values.dump() << "\n";
}


void CalibrationPlatformCreator::create_platform() const {

    simgrid::s4u::Engine::set_config("network/loopback-bw:1000000000000");

    // Create the top-level zone
    auto top_zone = simgrid::s4u::create_full_zone("global");


    // Create the ETP zone
    auto etp_zone = simgrid::s4u::create_floyd_zone("ETP")->set_parent(top_zone);

    // Create sg01 host
    auto sg01 = etp_zone->create_host("sg01.etp.kit.edu", (std::string)calibration_values["sg01_flops"]);
    sg01->set_core_count(24);
    sg01->set_property("type", "worker,cache");
    sg01->set_property("ram", "64GiB");
    auto sg01_disk = sg01->create_disk("ssd_cache1",
                                       (std::string)calibration_values["ssd_cache1_disk_read_bw"],
                                       (std::string)calibration_values["ssd_cache1_disk_write_bw"]);
    sg01_disk->set_property("size", "2TB");
    sg01_disk->set_property("mount", "/");

    // Create sg03 host
    auto sg03 = etp_zone->create_host("sg03.etp.kit.edu", (std::string)calibration_values["sg03_flops"]);
    sg03->set_core_count(12);
    sg03->set_property("type", "worker,cache");
    sg03->set_property("ram", "32GiB");
    auto sg03_disk = sg03->create_disk("ssd_cache1",
                                       (std::string)calibration_values["ssd_cache3_disk_read_bw"],
                                       (std::string)calibration_values["ssd_cache3_disk_write_bw"]);
    sg03_disk->set_property("size", "2TB");
    sg03_disk->set_property("mount", "/");

    // Create sg04 host
    auto sg04 = etp_zone->create_host("sg04.etp.kit.edu", (std::string)calibration_values["sg04_flops"]);
    sg04->set_core_count(12);
    sg04->set_property("type", "worker,cache");
    sg04->set_property("ram", "32GiB");
    auto sg04_disk = sg04->create_disk("ssd_cache1",
                                       (std::string)calibration_values["ssd_cache4_disk_read_bw"],
                                       (std::string)calibration_values["ssd_cache4_disk_write_bw"]);
    sg04_disk->set_property("size", "2TB");
    sg04_disk->set_property("mount", "/");

    // Create WMSHost
    auto wms_host = etp_zone->create_host("WMSHost", "10Gf");
    wms_host->set_property("type", "scheduler,executor");
    wms_host->set_property("ram", "16GB");

    // Create both routers
    auto etp_gateway = etp_zone->create_router("etpgateway");
    auto etp_switch = etp_zone->create_router("etpswitch");

    // Create all links
    auto etp_link_0 = etp_zone->create_link("etp_link0",(std::string)calibration_values["etp_link_to_switch_bandwidth"])->set_latency("0us");
    auto etp_link_1 = etp_zone->create_link("etp_link1",(std::string)calibration_values["etp_link_to_switch_bandwidth"])->set_latency("0us");
    auto etp_link_3 = etp_zone->create_link("etp_link3",(std::string)calibration_values["etp_link_to_switch_bandwidth"])->set_latency("0us");
    auto etp_link_4 = etp_zone->create_link("etp_link4",(std::string)calibration_values["etp_link_to_switch_bandwidth"])->set_latency("0us");
    auto etp_linkOut = etp_zone->create_link("etp_linkOut",(std::string)calibration_values["etp_linkOut_bandwidth"])->set_latency("0us");

    // Create all routes
    {
        simgrid::s4u::LinkInRoute network_link_in_route{etp_link_0};
        etp_zone->add_route(etp_switch, wms_host->get_netpoint(), nullptr, nullptr, {network_link_in_route});
    }
    {
        simgrid::s4u::LinkInRoute network_link_in_route{etp_link_1};
        etp_zone->add_route(etp_switch, sg01->get_netpoint(), nullptr, nullptr, {network_link_in_route});
    }
    {
        simgrid::s4u::LinkInRoute network_link_in_route{etp_link_3};
        etp_zone->add_route(etp_switch, sg03->get_netpoint(), nullptr, nullptr, {network_link_in_route});
    }
    {
        simgrid::s4u::LinkInRoute network_link_in_route{etp_link_4};
        etp_zone->add_route(etp_switch, sg04->get_netpoint(), nullptr, nullptr, {network_link_in_route});
    }
    {
        simgrid::s4u::LinkInRoute network_link_in_route{etp_linkOut};
        etp_zone->add_route(etp_gateway, etp_switch, nullptr, nullptr, {network_link_in_route});
    }

    etp_zone->seal();

    // Create the Remote zone
    auto remote_zone = simgrid::s4u::create_full_zone("Remote")->set_parent(top_zone);

    // Create the RemoteStorage host
    auto remote_storage = remote_zone->create_host("RemoteStorage", "1000Gf")->set_core_count(10);
    remote_storage->set_property("type","storage");
    auto hard_drive = remote_storage->create_disk("hard_drive",
                                                  (std::string)calibration_values["remote_storage_disk_read_bw"],
                                                  (std::string)calibration_values["remote_storage_disk_write_bw"]);
    hard_drive->set_property("size", "1PB");
    hard_drive->set_property("mount", "/");

    // Create the link
    auto etp_to_remote = remote_zone->create_link("etp_to_remote",(std::string)calibration_values["etp_to_remote_link_network_bandwidth"])->set_latency("0us");

    etp_to_remote->seal();

    // Finish up the top zone
    {
        simgrid::s4u::LinkInRoute network_link_in_route{etp_to_remote};
        top_zone->add_route(etp_zone->get_netpoint(),
                            remote_zone->get_netpoint(),
                            etp_gateway,
                            remote_storage->get_netpoint(),
                            {network_link_in_route});
    }

    top_zone->seal();

}