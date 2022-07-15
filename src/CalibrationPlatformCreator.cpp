#include "CalibrationPlatformCreator.h"
#include <wrench-dev.h>

/**
 * @brief Constructor
 * @param json_spec calibration platform spec in JSON
 */
CalibrationPlatformCreator::CalibrationPlatformCreator(nlohmann::json json_spec) {
    // TODO: Implement all variables
}


void CalibrationPlatformCreator::create_platform() const {

    simgrid::s4u::Engine::set_config("network/loopback-bw:1000000000000");

    // Create the top-level zone
    auto top_zone = simgrid::s4u::create_full_zone("global");


    // Create the ETP zone
    auto etp_zone = simgrid::s4u::create_floyd_zone("ETP")->set_parent(top_zone);

    // Create sg01 host
    auto sg01 = etp_zone->create_host("sg01.etp.kit.edu", "1969.583Mf");
    sg01->set_core_count(24);
    sg01->set_property("type", "worker,cache");
    sg01->set_property("ram", "64GiB");
    auto sg01_disk = sg01->create_disk("ssd_cache1",
                                       "0.75GBps",
                                       "0.75GBps");
    sg01_disk->set_property("size", "2TB");
    sg01_disk->set_property("mount", "/");

    // Create sg03 host
    auto sg03 = etp_zone->create_host("sg03.etp.kit.edu", "1971.000Mf");
    sg03->set_core_count(12);
    sg03->set_property("type", "worker,cache");
    sg03->set_property("ram", "32GiB");
    auto sg03_disk = sg03->create_disk("ssd_cache1",
                                       "0.75GBps",
                                       "0.75GBps");
    sg03_disk->set_property("size", "2TB");
    sg03_disk->set_property("mount", "/");

    // Create sg04 host
    auto sg04 = etp_zone->create_host("sg04.etp.kit.edu", "1966.674Mf");
    sg04->set_core_count(12);
    sg04->set_property("type", "worker,cache");
    sg04->set_property("ram", "32GiB");
    auto sg04_disk = sg04->create_disk("ssd_cache1",
                                       "0.75GBps",
                                       "0.75GBps");
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
    auto etp_link_0 = etp_zone->create_link("etp_link0","10Gbps")->set_latency("0us");
    auto etp_link_1 = etp_zone->create_link("etp_link1","10Gbps")->set_latency("0us");
    auto etp_link_3 = etp_zone->create_link("etp_link3","10Gbps")->set_latency("0us");
    auto etp_link_4 = etp_zone->create_link("etp_link4","10Gbps")->set_latency("0us");
    auto etp_linkOut = etp_zone->create_link("etp_linkOut","10Gbps")->set_latency("0us");

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
                                       "40GBps",
                                       "40GBps");
    hard_drive->set_property("size", "1PB");
    hard_drive->set_property("mount", "/");

    // Create the link
    auto etp_to_remote = remote_zone->create_link("etp_to_remote","100Gbps")->set_latency("0us");

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