#!/usr/bin/env python3
import argparse
import atexit
import csv
import json
import os
import sys
from collections import defaultdict
from statistics import mean, stdev, StatisticsError
from pathlib import Path
import re

import simcal as sc

toolsDir = Path(
    os.path.dirname(os.path.realpath(__file__)))  # Get path to THIS folder where the simulator lives

def dataLoader(scsn, fcsn, scfn, fcfn):
	#todo, handle raw files
	#todo, sane dir structure
    with open(scsn, 'r') as file:
        scsn = json.load(file)
    with open(fcsn, 'r') as file:
        fcsn = json.load(file)
    with open(scfn, 'r') as file:
        scfn = json.load(file)
    with open(fcfn, 'r') as file:
        fcfn = json.load(file)
    return scsn, fcsn, scfn, fcfn


class Simulator(sc.Simulator):
    def __init__(self, path):
        super().__init__()
        self.path = path

    def extract(self, outfile):
        hitrate_data = defaultdict(list)
        with open(outfile, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                hitrate_data[row["machine.name"]].append(row)
        return hitrate_data

    def run(self, env, args):
        # Args structure
        # {
        #     "platform",
        #     "hitrate",
        #     "xrootd_blocksize",
        #     "network_blocksize",
        #     "workload",
        #     "output"
        # }

        # self.bash(path, str(jargs))
        output=env.tmp_file()
        env.bash(self.path,
                 args=(
                     "--platform", args["platform"],
                     "--output-file", output,
                     "--workload-configurations", args["workload"][0],
                     "--dataset-configurations", args["workload"][1],
                     "--hitrate", args["hitrate"],
                     "--xrd-blocksize", args["xrootd_block"],
                     "--storage-buffer-size", args["network_blocksize"],
                     "--duplications", "48",
                     "--cfg=network/loopback-bw:100000000000000",
                     "--no-caching"
                 ),
                 std_in=())
        return self.extract(output)


class SamplePoint:
    def __init__(self, simulator,xml_template, hitrates, xrootd_blocksize, network_blocksize, workloads):
        self.simulator = simulator
        self.hitrates = hitrates
        self.xrootd_blocksize = xrootd_blocksize
        self.network_blocksize = network_blocksize
        self.workloads = workloads
        with open(xml_template, 'r') as f:
            self.template = f.read()

    def fill_template(self, env, args):
        # Get the command-line arguments
        xml_contents = self.template

        # Replace the placeholders in the XML file with the specified values
        xml_contents = re.sub(r'{cpu-speed}', str(args["cpuSpeed"]), xml_contents)
        xml_contents = re.sub(r'{read-speed}', str(args["cacheSpeed"]), xml_contents)
        xml_contents = re.sub(r'{link-speed}', str(args["internalNetworkSpeed"]), xml_contents)
        xml_contents = re.sub(r'{net-speed}', str(args["externalNetworkSpeed"]), xml_contents)

        platform = env.tmp_file(encoding='utf8')
        platform.write(xml_contents)
        platform.flush()
        return platform

    def restructure(self, inter):
        # inter[workload][hitrate]{jobtag,machine,[data]}
        # to
        # ret[workload][machine][hitrate]{data} 
        ret = defaultdict(dict)
        for workload, hitrates in inter.items():
            for hitrate, machines in hitrates.items():
                for machine, data in machines.items():
                    ret[workload][machine][hitrate] = data
        return ret

    def call_platform(self, env, args):
        inter = {}
        platform = self.fill_template(env, args)
        for workload in self.workloads:
            inter[workload] = {}
            for hitrate in self.hitrates:
                inter[workload][hitrate] = self.simulator({"workload":workload, "platform":platform, "hitrate":hitrate,"xrootd_block":self.xrootd_blocksize,"network_blocksize":self.network_blocksize}, env=env)
        platform.close()
        return self.restructure(inter)

    def __call__(self, args):
        env = sc.Environment()
        env.tmp_dir(".")
        scsn = self.call_platform(env, 
            {"cpuSpeed": args["cpuSpeed"],
             "cacheSpeed": args["disk"],
             "internalNetworkSpeed": args["internalNetwork"],
             "externalNetworkSpeed": args["externalSlowNetwork"]
             })
        fcsn = self.call_platform(env, 
            {"cpuSpeed": args["cpuSpeed"],
             "cacheSpeed": args["ramDisk"],
             "internalNetworkSpeed": args["internalNetwork"],
             "externalNetworkSpeed": args["externalSlowNetwork"]
             })
        fcfn = self.call_platform(env, 
            {"cpuSpeed": args["cpuSpeed"],
             "cacheSpeed": args["ramDisk"],
             "internalNetworkSpeed": args["internalNetwork"],
             "externalNetworkSpeed": args["externalFastNetwork"]
             })
        scfn = self.call_platform(env, 
            {"cpuSpeed": args["cpuSpeed"],
             "cacheSpeed": args["disk"],
             "internalNetworkSpeed": args["internalNetwork"],
             "externalNetworkSpeed": args["externalFastNetwork"]
             })
        return scsn, scfn, fcsn, fcfn


def loss(reference, simulated):
    return 0


# do whatever
data = dataLoader("../../DCSIM calibration Data/individualSlowRawData.json",
                  "../../DCSIM calibration Data/individualFastRawData.json",
                  "../../DCSIM calibration Data/duplicateSlowRawData.json",
                  "../../DCSIM calibration Data/duplicateFastRawData.json"
                  )
simulator = Simulator("dc-sim")
calibrator = sc.calibrators.Debug(sys.stdout)

calibrator.add_param("cpuSpeed", sc.parameter.Exponential(20, 40).format("%.2f flops"))
calibrator.add_param("ramdisk", sc.parameter.Exponential(20, 40).format("%.2f Bps"))
calibrator.add_param("disk", sc.parameter.Exponential(20, 40).format("%.2f Bps"))
calibrator.add_param("internalNetwork", sc.parameter.Exponential(20, 40).format("%.2f bps"))
calibrator.add_param("externalFastNetwork", sc.parameter.Exponential(20, 40).format("%.2f bps"))
calibrator.add_param("externalSlowNetwork", sc.parameter.Exponential(20, 40).format("%.2f bps"))

dataDir=toolsDir/"../data"
point = SamplePoint(simulator,"../data/platform-files/sgbatch_validation_template.xml", [1.0,0.9,0.8,0.7,0.6,0.5,0.4,0.3,0.2,0.1,0.0], 1_000_000_000, 0, ((dataDir/"dataset-configs/crown_ttbar_testjob.json",dataDir/"workload-configs/crown_ttbar_testjob.json"),))
calibrator.calibrate(point, loss, data)
