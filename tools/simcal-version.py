#!/usr/bin/env python3
import simcal as sc
import os
import argparse
import json
from collections import defaultdict
from statistics import mean, stdev, StatisticsError

import csv
import sys
import atexit


class DataLoader(sc.DataLoader):
    def __init__(self, scsn, fcsn, scfn, fcfn):
        with open(scsn, 'r') as file:
            self.scsn = json.load(file)
        with open(fcsn, 'r') as file:
            self.fcsn = json.load(file)
        with open(scfn, 'r') as file:
            self.scfn = json.load(file)
        with open(fcfn, 'r') as file:
            self.fcfn = json.load(file)

    def get_data(self):
        return self.scsn, self.fcsn, self.scfn, self.fcfn


class Simulator(sc.Simulator):
    def __init__(self, path, json_template, hitrates):
        super().__init__()
        self.path = path
        self.template = sc.JSONTemplate(file=json_template)
        self.hitrates = hitrates

    def run(self, env, args):
        env.tmp_dir()
        jargs = self.template.fill(args[0])
        # self.bash(path, str(jargs))
        sc.bash(self.path, args=(str(jargs), self.hitrates), std_in=())
        return extract(env.get_cwd())


# do whatever
data = DataLoader("../../DCSIM\ calibration\ Data/individualSlowRawData.json",
                  "../../DCSIM\ calibration\ Data/individualFastRawData.json",
                  "../../DCSIM\ calibration\ Data/duplicateSlowRawData.json",
                  "../../DCSIM\ calibration\ Data/duplicateFastRawData.json"
                  )
simulator = Simulator("tools/hitrateScanScript.sh", "../data/platform-files/sgbatch_validation_template.xml",
                      "1.0 0.9 0.8 0.7 0.6 0.5 0.4 0.3 0.2 0.1 0.0")
calibrator = sc.Grid()  # tbd
calibrator.add_param("cpuSpeed", "flops").exponential_range(20, 40)
calibrator.add_param("ramdisk", "Bps").exponential_range(20, 40)
calibrator.add_param("disk", "Bps").exponential_range(20, 40)
calibrator.add_param("internalNetwork", "bps").exponential_range(20, 40)
calibrator.add_param("externalFastNetwork", "bps").exponential_range(20, 40)
calibrator.add_param("externalSlowNetwork", "bps").exponential_range(20, 40)


class SamplePoint:
    def __init__(self, simulator):
        self.simulator = simulator

    def __call__(self, args):
        scsn = self.simulator((
            {"cpuSpeed": args["cpuSpeed"],
             "cacheSpeed": args["disk"],
             "internalNetworkSpeed": args["internalNetwork"],
             "externalNetworkSpeed": args["externalSlowNetwork"]
             }))
        fcsn = self.simulator((
            {"cpuSpeed": args["cpuSpeed"],
             "cacheSpeed": args["ramDisk"],
             "internalNetworkSpeed": args["internalNetwork"],
             "externalNetworkSpeed": args["externalSlowNetwork"]
             }))
        fcfn = self.simulator((
            {"cpuSpeed": args["cpuSpeed"],
             "cacheSpeed": args["ramDisk"],
             "internalNetworkSpeed": args["internalNetwork"],
             "externalNetworkSpeed": args["externalFastNetwork"]
             }))
        scfn = self.simulator((
            {"cpuSpeed": args["cpuSpeed"],
             "cacheSpeed": args["disk"],
             "internalNetworkSpeed": args["internalNetwork"],
             "externalNetworkSpeed": args["externalFastNetwork"]
             }))
        return scsn, scfn, fcsn, fcfn


def loss(reference, simulated):
    total = 0
    for pair in zip(reference, simulated):
        total += evaluate(pair[0], pair[1])
    return total


point = SamplePoint(simulator)
calibrator.calibrate(point, loss, data.get_data())


## helper functions
def calculate_stats(stats):
    try:
        avg = mean(stats)
        std = stdev(stats)
    except StatisticsError:
        if 'avg' not in locals():
            avg = float('inf')
        std = 0
    return {
        'average': avg,
        'stdev': std
    }


def extract(directory, log=None):
    extracted_results = []
    hitrate_data = defaultdict(dict)
    for file in os.listdir(directory):
        if file.endswith('.csv'):
            hitrate = file.split('_')[4][:-4]  ##remove .csv
            if hitrate == 'hitrate1' or hitrate == 'hitrate0':
                hitrate += ".0"
            stats = sc.parse_csv(os.path.join(directory, file))
            for machine, machine_stats in stats.items():
                if log:
                    # for stat in machine_stats:
                    extracted_results.append((log, hitrate, machine_stats))
                # print(((log,hitrate,machine_stats)))
                hitrate_data[hitrate][machine.strip()] = calculate_stats(machine_stats)

    return hitrate_data, extracted_results


def evaluate(run, reference):
    ret = 0
    count = 0
    for hitrate in run:
        for machine in run[hitrate]:
            count += 1
            # L1
            ret += abs(
                float(run[hitrate][machine]['average']) -
                float(reference[hitrate][machine]['average'])
            )
    if count == 0:
        return float('inf')
    return ret / count

def parse_csv(file):
    stats = defaultdict(list)
    with open(file, 'r') as f:
        reader = csv.DictReader(f)
        reader.fieldnames = [field.strip() for field in reader.fieldnames]
        for row in reader:
            machine = row['machine.name']
            runtime = float(row['job.end']) - float(row['job.start'])
            stats[machine].append(runtime)
    return stats