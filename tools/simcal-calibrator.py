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
import glob
import simcal as sc
import ddks#pip install git+https://github.com/pnnl/DDKS 
import torch #pip install torchvision
    

toolsDir = Path(
	os.path.dirname(os.path.realpath(__file__)))  # Get path to THIS folder where the simulator lives
def extract(file):
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
	def __init__(self, path):
		super().__init__()
		self.path = path

	

	def run(self, env, args):
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
		
		output=env.tmp_file(keep=False)
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
				 "--seed", 0
			 ]
		for i in range(len(cargs)):
			cargs[i]=str(cargs[i])
		#print('dc-sim', ' '.join(cargs))
		o=env.bash(self.path,
				 args=cargs)
		#print(o[1])
		return extract(output.name)


class SamplePoint:
	def __init__(self, simulator,xml_template, hitrates, xrootd_blocksize, network_blocksize, workloads,data):
		self.simulator = simulator
		self.hitrates = hitrates
		self.xrootd_blocksize = xrootd_blocksize
		self.network_blocksize = network_blocksize
		self.workloads = workloads
		self.data=data
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

		platform = env.tmp_file(encoding='utf8',keep=False)
		platform.write(xml_contents)
		platform.flush()
		return platform



	def call_platform(self, env, args):
		inter = {}
		platform = self.fill_template(env, args)
		for workload in self.workloads:
			inter[workload] = {}
			for hitrate in self.hitrates:
				inter[workload][hitrate] = self.simulator({"workload":self.workloads[workload], "platform":platform.name, "hitrate":hitrate,"xrootd_block":self.xrootd_blocksize,"network_blocksize":self.network_blocksize}, env=env)
		platform.close()
		return restructure(inter)

	def __call__(self, args, stop_time = None):
		
		env = sc.Environment(stoptime=stop_time)
		with env:
			env.tmp_dir(".",keep=False)
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
		#loss(self.data,(scsn,scfn,fcsn,fcfn))
		#loss(self.data,(scsn,scfn,fcsn,fcfn))
		return loss(self.data,(scsn,scfn,fcsn,fcfn))
def buildTensor(data):
	tensor=torch.empty((len(data),2), dtype=torch.float32)
	for i,data in enumerate(data):
		start=float(data['job.start'])
		end=float(data['job.end'])
		cpu=float(data['job.computetime'])
		#print(i,start,end,cpu)
		tensor[i,0]=end-start
		tensor[i,1]=(end-start)/cpu
	#print(tensor)
	return tensor
def loss(reference, simulated):
	calculation = ddks.methods.ddKS()
	count=0
	total=0
	for platform in zip(reference,simulated):
		for expiriment in sorted(platform[1].keys() & platform[0].keys()):
			sim=platform[1][expiriment]
			for ref in platform[0][expiriment]:
				for machine in sorted(sim.keys()&ref.keys()):
					for hitrate in sorted(sim[machine].keys()&ref[machine].keys()):
						#break
						#N dimensional KS test (ddks) 
						#unless we can find n dimensional k sample anderson darling
						#Apparently we are doing Wasserstein 
						#psych! we are doing ddKS
						refTensor=buildTensor(ref[machine][hitrate])
						
						simTensor=buildTensor(sim[machine][hitrate])
						#print(refTensor,simTensor)
						
						#print(type(ref[machine][hitrate]))
						#print(type(sim[machine][hitrate]))
						#print(len(ref[machine][hitrate]))
						#print(len(sim[machine][hitrate]))
						#print(sim[machine][hitrate])
						#print(ref[machine][hitrate])
						#print(refTensor,simTensor)
						total+=  float(calculation(refTensor,simTensor))
						#print(distance)
						count+=1
						#There are a different number of results for each machine in each dataset
						#return total
						#print("\t",expiriment,machine,hitrate,total,count)
	if(count==0):
		count=1
	print(total/count)
	return total/count

if __name__=="__main__":

	parser = argparse.ArgumentParser(description="Calibrate DCSim using simcal")
	parser.add_argument("-t", "--timelimit", type=int, required=True, help="Timelimit in seconds")
	parser.add_argument("-c", "--cores", type=int, required=True, help="Number of CPU cores")
	args = parser.parse_args()
	# do whatever
	data = dataLoader({"test":[
					  glob.glob(os.path.expanduser("~/hep-testjob/data/testjob/diskCache/SG*1Gbps*")),
					  glob.glob(os.path.expanduser("~/hep-testjob/data/testjob/ramCache/SG*1Gbps*")),
					  glob.glob(os.path.expanduser("~/hep-testjob/data/testjob/diskCache/SG*10Gbps*")),
					  glob.glob(os.path.expanduser("~/hep-testjob/data/testjob/ramCache/SG*10Ggps*"))],
					  
					  "copy":[
					  glob.glob(os.path.expanduser("~/hep-testjob/data/copyjob/diskCache/SG*1Gbps*")),
					  glob.glob(os.path.expanduser("~/hep-testjob/data/copyjob/ramCache/SG*1Gbps*")),
					  glob.glob(os.path.expanduser("~/hep-testjob/data/copyjob/diskCache/SG*10Gbps*")),
					  glob.glob(os.path.expanduser("~/hep-testjob/data/copyjob/ramCache/SG*10Ggps*"))]
					  })
		   
	simulator = Simulator("dc-sim")
	#calibrator = sc.calibrators.Debug(sys.stdout)
	#calibrator = sc.calibrators.Grid()
	#calibrator = sc.calibrators.Random()
	calibrator = sc.calibrators.GradientDescent(0.001,0.00001,early_reject_loss=1.0)
	calibrator.add_param("cpuSpeed", sc.parameter.Exponential(20, 40).format("%.2f"))
	calibrator.add_param("ramDisk", sc.parameter.Exponential(20, 40).format("%.2f"))
	calibrator.add_param("disk", sc.parameter.Exponential(20, 40).format("%.2f"))
	calibrator.add_param("internalNetwork", sc.parameter.Exponential(20, 40).format("%.2f"))
	calibrator.add_param("externalFastNetwork", sc.parameter.Exponential(20, 40).format("%.2f"))
	calibrator.add_param("externalSlowNetwork", sc.parameter.Exponential(20, 40).format("%.2f"))

	dataDir=toolsDir/"../data"
	samplePoint = SamplePoint(simulator,dataDir/"platform-files/sgbatch_validation_template.xml", [1.0,0.9,0.8,0.7,0.6,0.5,0.4,0.3,0.2,0.1,0.0], 10_000_000_000, 0, {"test":(dataDir/"dataset-configs/crown_ttbar_testjob.json",dataDir/"workload-configs/crown_ttbar_testjob.json"),"copy":(dataDir/"dataset-configs/crown_ttbar_copyjob.json",dataDir/"workload-configs/crown_ttbar_copyjob.json")},data)
	coordinator = sc.coordinators.ThreadPool(pool_size=args.cores) 
	maxs=samplePoint(	{"cpuSpeed":"1970Mf",	"disk":"17MBps", "ramDisk":"1GBps",	"internalNetwork":"10GBps",	"externalSlowNetwork":"1.15Gbps", "externalFastNetwork":"11.5Gbps"})
	print("Max's",maxs)
	cal=calibrator.calibrate(samplePoint, soft_timelimit=args.timelimit, coordinator=coordinator)
	print ("We should now be printing the calibration")
	print(cal)