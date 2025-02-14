#!/usr/bin/env python3
import argparse
import atexit
import csv
import tempfile
import json
import os
import sys
from collections import defaultdict
from statistics import mean, stdev, StatisticsError
from pathlib import Path
import re
import glob
import simcal as sc
import math


import time    
from skywalker import processify #pip install skywalker

toolsDir = Path(os.path.dirname(os.path.realpath(__file__)))  
# Get path to THIS folder where the simulator lives
def extract(file):
	print(file)
	hitrate_data = defaultdict(list)
	if os.stat(file).st_size == 0:
		raise RuntimeError("Simulation produced empty output file")
	with open(file, 'r') as f:
		reader = csv.DictReader(f)
		for row in reader:
			newRow={}
			for key in row:
				if key:
					newRow[key.strip()]=row[key].strip()
			row=newRow
			hitrate_data[row["machine.name"]].append(row)
	return hitrate_data
def restructure(inter):
	# inter[workload][hitrate]{jobtag,machine,[data]}
	# to
	# ret[workload][machine][hitrate]{data} 
	ret = {}
	for workload, hitrates in inter.items():
		if not workload in ret:
			ret[workload]={}
		for hitrate, machines in hitrates.items():
			#print(machines)
			for machine, data in machines.items():
				if not machine in ret[workload]:
					ret[workload][machine]={}
				ret[workload][machine][hitrate] = data
	return ret
def loadDirs(folders):
	ret={}
	for folder in folders:
		root,dirs,files=next(os.walk(folder))
		ret[root]={}
		for file in files:
			if "bad_" in file:
				#print("skipping",file)
				continue
			fileContent=extract(root+"/"+file)
			ret[root][float(file[file.rfind("_")+1:file.rfind(".")])]=fileContent
	return list(restructure(ret).values())
def dataLoader(sets):
	scsn={}
	fcsn={}
	scfn={}
	fcfn={}
	for dataset in sets:
		scsn[dataset]=loadDirs(sets[dataset][0])
		fcsn[dataset]=loadDirs(sets[dataset][1])
		scfn[dataset]=loadDirs(sets[dataset][2])
		fcfn[dataset]=loadDirs(sets[dataset][3])
	#print(fcsn)
	#print(scsn)
	#todo, handle raw files
	#todo, sane dir structure
	return scsn,scfn,fcsn,fcfn


class Simulator(sc.Simulator):
	def __init__(self, path,xml_template, hitrates, xrootd_blocksize, network_blocksize, workloads,data):
		super().__init__()
		self.path = path
		self.hitrates = hitrates
		self.xrootd_blocksize = xrootd_blocksize
		self.network_blocksize = network_blocksize
		self.workloads = workloads
		self.data=data
		with open(xml_template, 'r') as f:
			self.template = f.read()


	def dcsim(self, env, args):
		# Args structure
		# {
		#	 "platform",
		#	 "hitrate",
		#	 "xrootd_blocksize",
		#	 "network_blocksize",
		#	 "workload",
		#	 "output"
		# }

		# self.bash(path, str(jargs))
		
		output=args["output"]
		cargs=[
				 "--platform", args["platform"],
				 "--output-file", output.name,
				 "--workload-configurations", args["workload"][1],
				 "--dataset-configurations", args["workload"][0],
				 "--hitrate", args["hitrate"],
				 "--xrd-blocksize", args["xrootd_block"],
				 "--storage-buffer-size", args["network_blocksize"],
				 "--duplications", "48",
				 "--cfg=network/loopback-bw:100000000000000",
				 "--no-caching",
				 "--seed", 0,
				 "--xrd-flops-per-time",args["xrootd_flops"]
			 ]
		for i in range(len(cargs)):
			cargs[i]=str(cargs[i])
		#print('dc-sim', ' '.join(cargs))
		o=env.bash(self.path,
				 args=cargs)
		#print(o[1])
		return (extract(output),o[1])
	

	def fill_template(self, env, args):
		# Get the command-line arguments
		xml_contents = self.template

		# Replace the placeholders in the XML file with the specified values
		xml_contents = re.sub(r'{cpu-speed}', str(args["cpuSpeed"]), xml_contents)
		xml_contents = re.sub(r'{read-speed}', str(args["cacheSpeed"]), xml_contents)
		xml_contents = re.sub(r'{link-speed}', str(args["internalNetworkSpeed"]), xml_contents)
		xml_contents = re.sub(r'{net-speed}', str(args["externalNetworkSpeed"]), xml_contents)

		platform = env.tmp_file(encoding='utf8',keep=False)
		platform.write(xml_contents)
		platform.flush()
		return platform



	def call_platform(self, env, args):
		inter = {}
		out = {}
		platform = self.fill_template(env, args)
		for workload in self.workloads:

			for hitrate in self.hitrates:
				#print(workload,hitrate,)
				os.makedirs(args["output"]/workload/args["cacheName"]/args["SGname"], exist_ok=True)
				i,o=self.dcsim(env,{"workload":self.workloads[workload], "platform":platform.name, "hitrate":hitrate,"xrootd_block":self.xrootd_blocksize,"network_blocksize":self.network_blocksize,"xrootd_flops":args["xrootd_flops"],"output":args["output"]/workload/args["cacheName"]/args["SGname"]/("synthetic_hitrate_"+str(hitrate)+".csv")})

		platform.close()

	def run(self, env, iargs):
		args=dict(iargs)
		
		#with env:
		env.tmp_dir(tempfile.gettempdir(),keep=False)
		
		#scsn = 
		self.call_platform(env, 
			{"cpuSpeed": args["cpuSpeed"],
			 "cacheSpeed": args["disk"],
			 "internalNetworkSpeed": args["internalNetwork"],
			 "externalNetworkSpeed": args["externalSlowNetwork"],
			 "xrootd_flops":args["xrootd_flops"],
			 "output":args["output"],
			 "cacheName":"diskCache",
			 "SGname":"SG1_Synthetic1Gbps"
			 })
		#fcsn = 
		self.call_platform(env, 
			{"cpuSpeed": args["cpuSpeed"],
			 "cacheSpeed": args["ramDisk"],
			 "internalNetworkSpeed": args["internalNetwork"],
			 "externalNetworkSpeed": args["externalSlowNetwork"],
			 "xrootd_flops":args["xrootd_flops"],
			 "output":args["output"],
			 "cacheName":"ramCache",
			 "SGname":"SG1_Synthetic1Gbps"
			 })
		#fcfn = 
		self.call_platform(env, 
			{"cpuSpeed": args["cpuSpeed"],
			 "cacheSpeed": args["ramDisk"],
			 "internalNetworkSpeed": args["internalNetwork"],
			 "externalNetworkSpeed": args["externalFastNetwork"],
			 "xrootd_flops":args["xrootd_flops"],
			 "output":args["output"],
			 "cacheName":"ramCache",
			 "SGname":"SG1_Synthetic10Gbps"
			 })
		#scfn = 
		self.call_platform(env, 
			{"cpuSpeed": args["cpuSpeed"],
			 "cacheSpeed": args["disk"],
			 "internalNetworkSpeed": args["internalNetwork"],
			 "externalNetworkSpeed": args["externalFastNetwork"],
			 "xrootd_flops":args["xrootd_flops"],
			 "output":args["output"],
			 "cacheName":"diskCache",
			 "SGname":"SG1_Synthetic10Gbps"
			 })
		#loss(self.data,(scsn,scfn,fcsn,fcfn))
		#loss(self.data,(scsn,scfn,fcsn,fcfn))
		


if __name__=="__main__":

	parser = argparse.ArgumentParser(description="Calibrate DCSim using simcal")
	parser.add_argument("-g", "--groundtruth", type=str, required=True, help="Ground Truth data folder")
	parser.add_argument("-o", "--output", type=str, required=True, help="folder to put the new synthetic data in")
	parser.add_argument("-c", "--cores", type=int, required=True, help="Number of CPU cores")
	parser.add_argument("-a", "--args", type=str, required=True, help="args to gen synthetic data for")
	args = parser.parse_args()
	
	
	data = dataLoader({"test":[
					  glob.glob(os.path.expanduser(f"{args.groundtruth}/data/testjob/diskCache/SG*1Gbps*")),
					  glob.glob(os.path.expanduser(f"{args.groundtruth}/data/testjob/ramCache/SG*1Gbps*")),
					  glob.glob(os.path.expanduser(f"{args.groundtruth}/data/testjob/diskCache/SG*10Gbps*")),
					  glob.glob(os.path.expanduser(f"{args.groundtruth}/data/testjob/ramCache/SG*10Gbps*"))],
					  
					  "copy":[
					  glob.glob(os.path.expanduser(f"{args.groundtruth}/data/copyjob/diskCache/SG*1Gbps*")),
					  glob.glob(os.path.expanduser(f"{args.groundtruth}/data/copyjob/ramCache/SG*1Gbps*")),
					  glob.glob(os.path.expanduser(f"{args.groundtruth}/data/copyjob/diskCache/SG*10Gbps*")),
					  glob.glob(os.path.expanduser(f"{args.groundtruth}/data/copyjob/ramCache/SG*10Gbps*"))]
					  })
	

	dataDir=toolsDir/"../data"
	simulator = Simulator("dc-sim",dataDir/"platform-files/sgbatch_validation_template.xml", 
		[1.0,0.9,0.8,0.7,0.6,0.5,0.4,0.3,0.2,0.1,0.0], 10_000_000_000, 0, 
		{"test":(dataDir/"dataset-configs/crown_ttbar_testjob.json",
		dataDir/"workload-configs/crown_ttbar_testjob.json"),
		"copy":(dataDir/"dataset-configs/crown_ttbar_copyjob.json",
		dataDir/"workload-configs/crown_ttbar_copyjob_no_cpu.json")},
		data)	
	
	coordinator = sc.coordinators.ThreadPool(pool_size=args.cores) 
	sargs=eval(args.args)
	sargs["output"]=Path(args.output)
	simulator(sargs)
	
	#t0 = time.time()
	#cal=calibrator.calibrate(simulator, timelimit=args.timelimit, coordinator=coordinator)
	#t1 = time.time()
	#print ("We should now be printing the calibration")
	#print(cal)
	#print(t1-t0)

#./generate_synthetic.py -g "$(subRoot.sh)/hep-testjob-copy" -o "$(subRoot.sh)/synthetic" -c $(nproc) -a "{'cpuSpeed': 1950000000, 'ramDisk': 27000000000, 'disk': 23000000, 'internalNetwork': 1900000000, 'xrootd_flops': 1000000000000, 'externalFastNetwork': 4000000000, 'externalSlowNetwork': 218000000}"	