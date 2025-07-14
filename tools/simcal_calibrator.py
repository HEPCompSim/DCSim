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
import ddks
import torch
import matplotlib.pyplot as plt
#from pytorch3d.loss import chamfer_distance
#conda install pytorch3d -c pytorch3d
from scipy.spatial.distance import directed_hausdorff
import ot
from sklearn.metrics import mean_squared_error, mean_absolute_error


import time    
from skywalker import processify

toolsDir = Path(os.path.dirname(os.path.realpath(__file__)))  
# Get path to THIS folder where the simulator lives
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
	def __init__(self, path,xml_template, hitrates, xrootd_blocksize, network_blocksize, workloads,data,loss,nocpu,ratio,plot=False):
		super().__init__()
		self.path = path
		self.hitrates = hitrates
		self.xrootd_blocksize = xrootd_blocksize
		self.network_blocksize = network_blocksize
		self.workloads = workloads
		self.data=data
		self.loss=loss
		self.nocpu=nocpu
		self.ratio=ratio
		self.plot=plot
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
				 "--seed", 0,
				 "--xrd-flops-per-time",args["xrootd_flops"]
			 ]
		for i in range(len(cargs)):
			cargs[i]=str(cargs[i])
		#print('dc-sim', ' '.join(cargs))
		o=env.bash(self.path,
				 args=cargs)
		#print(o[1])
		return (extract(output.name),o[1])
	

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
			inter[workload] = {}
			out[workload] = {}
			for hitrate in self.hitrates:
				i,o=self.dcsim(env,{"workload":self.workloads[workload], "platform":platform.name, "hitrate":hitrate,"xrootd_block":self.xrootd_blocksize,"network_blocksize":self.network_blocksize,"xrootd_flops":args["xrootd_flops"]})
				inter[workload][hitrate] = i
				out[workload][hitrate] = o
		platform.close()
		return restructure(inter),out

	def run(self, env, iargs):
		args=dict(iargs)
		if self.nocpu:
			args["cpuSpeed"]="1960000000"
		if self.ratio:
			args["externalFastNetwork"]=args["externalNetwork"]*self.ratio
			args["externalSlowNetwork"]=args["externalNetwork"]
		#with env:
		env.tmp_dir(tempfile.gettempdir(),keep=False)
		
		scsn = self.call_platform(env, 
			{"cpuSpeed": args["cpuSpeed"],
			 "cacheSpeed": args["disk"],
			 "internalNetworkSpeed": args["internalNetwork"],
			 "externalNetworkSpeed": args["externalSlowNetwork"],
			 "xrootd_flops":args["xrootd_flops"]
			 })
		fcsn = self.call_platform(env, 
			{"cpuSpeed": args["cpuSpeed"],
			 "cacheSpeed": args["ramDisk"],
			 "internalNetworkSpeed": args["internalNetwork"],
			 "externalNetworkSpeed": args["externalSlowNetwork"],
			 "xrootd_flops":args["xrootd_flops"]
			 })
		fcfn = self.call_platform(env, 
			{"cpuSpeed": args["cpuSpeed"],
			 "cacheSpeed": args["ramDisk"],
			 "internalNetworkSpeed": args["internalNetwork"],
			 "externalNetworkSpeed": args["externalFastNetwork"],
			 "xrootd_flops":args["xrootd_flops"]
			 })
		scfn = self.call_platform(env, 
			{"cpuSpeed": args["cpuSpeed"],
			 "cacheSpeed": args["disk"],
			 "internalNetworkSpeed": args["internalNetwork"],
			 "externalNetworkSpeed": args["externalFastNetwork"],
			 "xrootd_flops":args["xrootd_flops"]
			 })
		#loss(self.data,(scsn,scfn,fcsn,fcfn))
		#loss(self.data,(scsn,scfn,fcsn,fcfn))
		if self.plot:
			plot(self.data,(scsn[0],scfn[0],fcsn[0],fcfn[0]))
			plotCPU(self.data,(scsn[0],scfn[0],fcsn[0],fcfn[0]))
		loss = self.loss(self.data,(scsn[0],scfn[0],fcsn[0],fcfn[0]))
		if loss== float("inf"):
			print("infinte loss")
			print(args)
			print(scsn[1])
			print(scfn[1])
			print(fcsn[1])
			print(fcfn[1])
			raise sc.exception.InvalidSimulation("A simulation resulted in an infinte loss value",(scsn[1],scfn[1],fcsn[1],fcfn[1]))
		return loss
		
def plot(reference,simulated):
	index=0
	for platform in zip(reference,simulated):
		index+=1
		for expiriment in sorted(platform[1].keys() & platform[0].keys()):
			#print(expiriment)
			sim=platform[1][expiriment]
			sim_means=[]
			sim_stds=[]
			ref_means=[]
			ref_stds=[]
			ref_hitrates=[]
			sim_hitrates=[]
			reorg={}

			for ref in platform[0][expiriment]:
				for machine in sorted(ref.keys()):
					if not machine in reorg:
						reorg[machine]={}
					for hitrate in sorted(ref[machine].keys()):
						if not hitrate in reorg[machine]:
							reorg[machine][hitrate]=[]
						
						ref_data = ref[machine][hitrate]
						ref_times = [(float(entry['job.end']) - float(entry['job.start']))/60 for entry in ref_data]
						
						reorg[machine][hitrate]+=ref_times
						
						
						# Calculate job times for reference data
			for machine in sorted(reorg.keys()):
				for hitrate in sorted(reorg[machine].keys()):
					ref_data = reorg[machine][hitrate]
					ref_hitrates.append(hitrate)
					ref_mean = np.mean(ref_data)
					ref_std = np.std(ref_data)
					
					ref_means.append(ref_mean)
					ref_stds.append(ref_std)
			for machine in sorted(sim.keys()):
				for hitrate in sorted(sim[machine].keys()):
					sim_hitrates.append(hitrate)
					sim_data = sim[machine][hitrate]
					# Calculate job times for simulated data
					sim_times = [(float(entry['job.end']) - float(entry['job.start']))/60 for entry in sim_data]
					sim_mean = np.mean(sim_times)
					sim_std = np.std(sim_times)
					sim_means.append(sim_mean)
					sim_stds.append(sim_std)
				
			#print("")			
			# Create plot
			plt.figure()
			#print(reorg)
			#print(ref_hitrates)
			#print(ref_means)
			#print(expiriment)
			#print(sim_means)
			plt.errorbar(ref_hitrates, ref_means, yerr=ref_std, fmt='o', label='Reference')
			plt.errorbar(sim_hitrates, sim_means, yerr=sim_stds, fmt='o', label='Simulated')
			
			plt.xlabel('Hitrate')
			plt.ylabel('Job Time')
			plt.title(f'Platform {index} - {expiriment}')
			plt.legend()
			
			# Save plot to file
			plt.savefig(f'platform_{index}_{expiriment}.png')
			plt.close()
def plotCPU(reference,simulated):
	index=0
	for platform in zip(reference,simulated):
		index+=1
		for expiriment in sorted(platform[1].keys() & platform[0].keys()):
			#print(expiriment)
			sim=platform[1][expiriment]
			sim_means=[]
			sim_stds=[]
			ref_means=[]
			ref_stds=[]
			ref_hitrates=[]
			sim_hitrates=[]
			reorg={}

			for ref in platform[0][expiriment]:
				for machine in sorted(ref.keys()):
					if not machine in reorg:
						reorg[machine]={}
					for hitrate in sorted(ref[machine].keys()):
						if not hitrate in reorg[machine]:
							reorg[machine][hitrate]=[]
						
						ref_data = ref[machine][hitrate]
						ref_times = [float(entry['job.computetime'])/60 for entry in ref_data]
						
						reorg[machine][hitrate]+=ref_times
						
						
						# Calculate job times for reference data
			for machine in sorted(reorg.keys()):
				for hitrate in sorted(reorg[machine].keys()):
					ref_data = reorg[machine][hitrate]
					ref_hitrates.append(hitrate)
					ref_mean = np.mean(ref_data)
					ref_std = np.std(ref_data)
					
					ref_means.append(ref_mean)
					ref_stds.append(ref_std)
			for machine in sorted(sim.keys()):
				for hitrate in sorted(sim[machine].keys()):
					sim_hitrates.append(hitrate)
					sim_data = sim[machine][hitrate]
					# Calculate job times for simulated data
					sim_times = [float(entry['job.computetime'])/60 for entry in sim_data]
					sim_mean = np.mean(sim_times)
					sim_std = np.std(sim_times)
					sim_means.append(sim_mean)
					sim_stds.append(sim_std)
				
			#print("")			
			# Create plot
			plt.figure()
			#print(reorg)
			#print(ref_hitrates)
			#print(ref_means)
			#print(expiriment)
			#print(sim_means)
			plt.errorbar(ref_hitrates, ref_means, yerr=ref_std, fmt='o', label='Reference')
			plt.errorbar(sim_hitrates, sim_means, yerr=sim_stds, fmt='o', label='Simulated')
			
			plt.xlabel('Hitrate')
			plt.ylabel('Job Time')
			plt.title(f'Platform {index} - {expiriment} - CPU')
			plt.legend()
			
			# Save plot to file
			plt.savefig(f'platform_{index}_{expiriment} - CPU.png')
			plt.close()	
def buildTensor(data):
	tensor=torch.empty((len(data),2), dtype=torch.float32)
	for i,data in enumerate(data):
		start=float(data['job.start'])
		end=float(data['job.end'])
		cpu=float(data['job.computetime'])
		#print(i,start,end,cpu)
		tensor[i,0]=end-start
		tensor[i,1]=cpu
	#print(tensor)
	return tensor
@processify	
def MRELoss(reference, simulated):
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
						refTime=0
						for data in ref[machine][hitrate]:
							refTime+=float(data['job.end'])-float(data['job.start'])
						refTime/=len(ref[machine][hitrate])
						simTime=0
						for data in sim[machine][hitrate]:
							simTime+=float(data['job.end'])-float(data['job.start'])
						simTime/=len(sim[machine][hitrate])
						#print(refTensor,simTensor)
						
						#print(type(ref[machine][hitrate]))
						#print(type(sim[machine][hitrate]))
						#print(len(ref[machine][hitrate]))
						#print(len(sim[machine][hitrate]))
						#print(sim[machine][hitrate])
						#print(ref[machine][hitrate])
						#print(refTensor,simTensor)
						total+=  abs(refTime-simTime)/refTime
						#print(distance)
						count+=1
						#There are a different number of results for each machine in each dataset
						#return total
						#print("\t",expiriment,machine,hitrate,total,count)
	if(count==0):
		count=1
		return float('inf')
	#print(total/count)
	return total/count
@processify	
def MRELossRatio(reference, simulated):
	count=0
	total=0
	for platform in zip(reference,simulated):
		for expiriment in sorted(platform[1].keys() & platform[0].keys()):
			sim=platform[1][expiriment]
			for ref in platform[0][expiriment]:
				for machine in sorted(sim.keys()&ref.keys()):
					for hitrate in sorted(sim[machine].keys()&ref[machine].keys()):
						refTime=0
						refRatio=0
						for data in ref[machine][hitrate]:
							time=float(data['job.end'])-float(data['job.start'])
							cpu=float(data['job.computetime'])
							refTime+=time
							refRatio+=cpu
						refRatio/=len(ref[machine][hitrate])
						refTime/=len(ref[machine][hitrate])
						simTime=0
						simRatio=0
						for data in sim[machine][hitrate]:
							time=float(data['job.end'])-float(data['job.start'])
							cpu=float(data['job.computetime'])
							simTime+=time
							simRatio+=cpu
						simTime/=len(sim[machine][hitrate])
						simRatio/=len(sim[machine][hitrate])

						total+=  abs(refTime-simTime)/refTime+abs(simRatio-refRatio)

						count+=1

	if(count==0):
		count=1
		return float('inf')
	#print(total/count)
	return total/count
@processify	
def ddksLoss(reference, simulated):
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
		return float('inf')
	#print(total/count)
	return total/count
import numpy as np
from scipy.spatial import KDTree

def chamfer_distance(A, B):
    """
    Computes the chamfer distance between two sets of points A and B.
    """
    tree = KDTree(B)
    dist_A = tree.query(A)[0]
    tree = KDTree(A)
    dist_B = tree.query(B)[0]
    return np.mean(dist_A) + np.mean(dist_B)
	
@processify	
def chamferLoss(reference, simulated):
	calculation = chamfer_distance
	count=0
	total=0
	for platform in zip(reference,simulated):
		for expiriment in sorted(platform[1].keys() & platform[0].keys()):
			sim=platform[1][expiriment]
			for ref in platform[0][expiriment]:
				for machine in sorted(sim.keys()&ref.keys()):
					for hitrate in sorted(sim[machine].keys()&ref[machine].keys()):
						refTensor=buildTensor(ref[machine][hitrate])
						simTensor=buildTensor(sim[machine][hitrate])
						total+=  float(calculation(refTensor,simTensor))
						count+=1
	if(count==0):
		count=1
		return float('inf')
	#print(total/count)
	return total/count
@processify	
def hausdorffLoss(reference, simulated):
	calculation = directed_hausdorff
	count=0
	total=0
	for platform in zip(reference,simulated):
		for expiriment in sorted(platform[1].keys() & platform[0].keys()):
			sim=platform[1][expiriment]
			for ref in platform[0][expiriment]:
				for machine in sorted(sim.keys()&ref.keys()):
					for hitrate in sorted(sim[machine].keys()&ref[machine].keys()):
						refTensor=buildTensor(ref[machine][hitrate])
						simTensor=buildTensor(sim[machine][hitrate])
						total+=  float(calculation(refTensor,simTensor)[0])
						count+=1
	if(count==0):
		count=1
		return float('inf')
	#print(total/count)
	return total/count	
@processify	
def wassersteinLoss(reference, simulated):
	calculation = ot.sliced.max_sliced_wasserstein_distance
	count=0
	total=0
	for platform in zip(reference,simulated):
		for expiriment in sorted(platform[1].keys() & platform[0].keys()):
			sim=platform[1][expiriment]
			for ref in platform[0][expiriment]:
				for machine in sorted(sim.keys()&ref.keys()):
					for hitrate in sorted(sim[machine].keys()&ref[machine].keys()):
						refTensor=buildTensor(ref[machine][hitrate])
						simTensor=buildTensor(sim[machine][hitrate])
						total+=  float(calculation(refTensor,simTensor))
						count+=1
	if(count==0):
		count=1
		return float('inf')
	#print(total/count)
	return total/count		
@processify	
def sortedMRELoss(reference, simulated):
	count=0
	total=0
	for platform in zip(reference,simulated):
		for expiriment in sorted(platform[1].keys() & platform[0].keys()):
			sim=platform[1][expiriment]
			for ref in platform[0][expiriment]:
				for machine in sorted(sim.keys()&ref.keys()):
					for hitrate in sorted(sim[machine].keys()&ref[machine].keys()):
						refTime=[]
						refRatio=[]
						for data in sorted(ref[machine][hitrate],key=lambda item: float(item['job.end'])-float(item['job.start'])):
							time=float(data['job.end'])-float(data['job.start'])
							cpu=float(data['job.computetime'])
							refTime.append(time)
							refRatio.append(cpu)
						#refRatio/=len(ref[machine][hitrate])
						#refTime/=len(ref[machine][hitrate])
						simTime=[]
						simRatio=[]
						for data in sorted(sim[machine][hitrate],key=lambda item: float(item['job.end'])-float(item['job.start'])):
							time=float(data['job.end'])-float(data['job.start'])
							cpu=float(data['job.computetime'])
							simTime.append(time)
							simRatio.append(cpu)
						#simTime/=len(sim[machine][hitrate])
						#simRatio/=len(sim[machine][hitrate])

						total+=  mean_absolute_error(refTime,simTime)
						total+=  mean_absolute_error(refRatio,simRatio)

						count+=1

	if(count==0):
		count=1
		return float('inf')
	#print(total/count)
	return total/count
@processify	
def doubleSortedMRELoss(reference, simulated):
	count=0
	total=0
	for platform in zip(reference,simulated):
		for expiriment in sorted(platform[1].keys() & platform[0].keys()):
			sim=platform[1][expiriment]
			for ref in platform[0][expiriment]:
				for machine in sorted(sim.keys()&ref.keys()):
					for hitrate in sorted(sim[machine].keys()&ref[machine].keys()):
						refTime=[]
						refRatio=[]
						for data in sorted(ref[machine][hitrate],key=lambda item: float(item['job.end'])-float(item['job.start'])):
							time=float(data['job.end'])-float(data['job.start'])
							refTime.append(time)
						for data in sorted(ref[machine][hitrate],key=lambda item: (float(item['job.end'])-float(item['job.start']))/max(1,float(data['job.computetime']))):
							time=float(data['job.end'])-float(data['job.start'])
							cpu=float(data['job.computetime'])
							refRatio.append(cpu)
						#refRatio/=len(ref[machine][hitrate])
						#refTime/=len(ref[machine][hitrate])
						simTime=[]
						simRatio=[]
						for data in sorted(sim[machine][hitrate],key=lambda item: float(item['job.end'])-float(item['job.start'])):
							time=float(data['job.end'])-float(data['job.start'])
							simTime.append(time)
						for data in sorted(sim[machine][hitrate],key=lambda item: (float(item['job.end'])-float(item['job.start']))/max(1,float(data['job.computetime']))):
							time=float(data['job.end'])-float(data['job.start'])
							cpu=float(data['job.computetime'])
							simRatio.append(cpu)
						#simTime/=len(sim[machine][hitrate])
						#simRatio/=len(sim[machine][hitrate])

						total+=  mean_absolute_error(refTime,simTime)
						total+=  mean_absolute_error(refRatio,simRatio)

						count+=1

	if(count==0):
		count=1
		return float('inf')
	#print(total/count)
	return total/count
if __name__=="__main__":

	parser = argparse.ArgumentParser(description="Calibrate DCSim using simcal")
	parser.add_argument("-g", "--groundtruth", type=str, required=True, help="Ground Truth data folder")
	parser.add_argument("-a", "--alg", type=str, required=True, help="Algorithm to use [grad|skopt.gp|skopt.gbrt|skopt.et|skopt.rf|random]")
	parser.add_argument("-t", "--timelimit", type=int, required=True, help="Timelimit in seconds")
	parser.add_argument("-c", "--cores", type=int, required=True, help="Number of CPU cores")
	parser.add_argument("-l", "--loss", type=str, required=True, help="Ground Truth data folder", default = "ddks")
	parser.add_argument('--nocpu', action='store_true', help="Dont calibrate CPU, instead use 1960Mf" )
	parser.add_argument("-r", "--networkratio", type=float, help="The ratio between slow and fast external network")
	parser.add_argument("-e", "--evaluate", type=str, help="Dont calibrate, just evaluate the provided arg dict")
	parser.add_argument('--plot', action='store_true', help="If Evaluating, generate a plot")
	parser.add_argument('--hyper_test', action='store_true', help="Run a new gradient descent starting from the point with various hyper parameters")
	parser.add_argument("-htl", "--hyper_test_low", type=float, help="The low bound of the hyper parameter test")
	parser.add_argument("-hth", "--hyper_test_high", type=float, help="The upper bound of the hyper parameter test")
	
	args = parser.parse_args()


	gdp=0
	er=None
	if args.loss=="mre":
		loss=MRELoss
		gdp=0.01
	elif args.loss=="ddks":
		#calibrator = sc.calibrators.GradientDescent(0.001,0.00001,early_reject_loss=1.0)
		gdp=0.001
		er=1.0
		loss=ddksLoss
	elif args.loss=="ratio":
		#calibrator = sc.calibrators.GradientDescent(0.001,0.00001,early_reject_loss=1.0)
		gdp=0.01
		loss=MRELossRatio
	elif args.loss== "chamfer":
		gdp=1
		loss=chamferLoss
	elif args.loss== "hausdorff":
		gdp=1
		loss=hausdorffLoss
	elif args.loss== "wasserstein":
		gdp=1
		loss=wassersteinLoss
	elif args.loss== "sorted":
		gdp=1
		loss=sortedMRELoss
	elif args.loss== "double":
		gdp=1
		loss=doubleSortedMRELoss
	else:
		print("unrecognized loss function",args.loss)
		sys.exit()
	calibrator=None
	if args.alg == "grad":
		calibrator = sc.calibrators.GradientDescent(0.01, gdp, early_reject_loss=er)
	elif args.alg == "skopt.gp":
		calibrator = sc.calibrators.ScikitOptimizer(1000,"GP",seed=0)
	elif args.alg == "skopt.gbrt":
		calibrator = sc.calibrators.ScikitOptimizer(1000,"GBRT",seed=0)
	elif args.alg == "skopt.et":
		calibrator = sc.calibrators.ScikitOptimizer(1000,"ET",seed=0)
	elif args.alg == "skopt.rf":
		calibrator = sc.calibrators.ScikitOptimizer(1000,"RF",seed=0)
	elif args.alg == "random":
		calibrator = sc.calibrators.Random(seed=0)
	else:
		print("unrecognized calibrator alg function",args.alg)
		sys.exit()
	# do whatever
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
	

	#calibrator = sc.calibrators.Debug(sys.stdout)
	#calibrator = sc.calibrators.Grid()
	#calibrator = sc.calibrators.Random()(0.01, 0.001) 0.9656790133317311
	if not args.nocpu:
		calibrator.add_param("cpuSpeed", sc.parameter.Exponential(20, 40).format("%.2f"))
	calibrator.add_param("ramDisk", sc.parameter.Exponential(20, 40).format("%.2f"))
	calibrator.add_param("disk", sc.parameter.Exponential(20, 33).format("%.2f"))
	calibrator.add_param("internalNetwork", sc.parameter.Exponential(20, 33).format("%.2f"))
	calibrator.add_param("xrootd_flops", sc.parameter.Exponential(20, 47).format("%.2f"))
	
	if args.networkratio:
		calibrator.add_param("externalNetwork", sc.parameter.Exponential(20, 33).format("%.2f"))
	else:
		calibrator.add_param("externalFastNetwork", sc.parameter.Exponential(20, 33).format("%.2f"))
		calibrator.add_param("externalSlowNetwork", sc.parameter.Exponential(20, 33).format("%.2f"))

	dataDir=toolsDir/"../data"
	simulator = Simulator("dc-sim",dataDir/"platform-files/sgbatch_validation_template.xml", 
		[1.0,0.9,0.8,0.7,0.6,0.5,0.4,0.3,0.2,0.1,0.0], 10_000_000_000, 0, 
		{"test":(dataDir/"dataset-configs/crown_ttbar_testjob.json",
		dataDir/"workload-configs/crown_ttbar_testjob.json"),
		"copy":(dataDir/"dataset-configs/crown_ttbar_copyjob.json",
		dataDir/"workload-configs/crown_ttbar_copyjob_no_cpu.json")},
		data,loss,False,False)	
	
	coordinator = sc.coordinators.ThreadPool(pool_size=args.cores) 
	maxs=simulator(	{"cpuSpeed":"1970Mf",	"disk":"17MBps", "ramDisk":"1GBps",	"internalNetwork":"10Gbps","externalNetwork":"1.15Gbps","externalSlowNetwork":"1.15Gbps", "externalFastNetwork":"11.5Gbps","xrootd_flops":20000000000})
	print("Max's",maxs)
	if args.evaluate:
		print(args.evaluate)
		simulator = Simulator("dc-sim",dataDir/"platform-files/sgbatch_validation_template.xml", 
			[1.0,0.9,0.8,0.7,0.6,0.5,0.4,0.3,0.2,0.1,0.0], 10_000_000_000, 0, 
			{"test":(dataDir/"dataset-configs/crown_ttbar_testjob.json",
			dataDir/"workload-configs/crown_ttbar_testjob.json"),
			"copy":(dataDir/"dataset-configs/crown_ttbar_copyjob.json",
			dataDir/"workload-configs/crown_ttbar_copyjob_no_cpu.json")},
			data,loss,args.nocpu,args.networkratio,args.plot)	
		result=simulator(eval(args.evaluate))
		print("Evaluation",result)
		if args.hyper_test:
			best=None
			bestLoss=None
			simulator = Simulator("dc-sim",dataDir/"platform-files/sgbatch_validation_template.xml", 
				[1.0,0.9,0.8,0.7,0.6,0.5,0.4,0.3,0.2,0.1,0.0], 10_000_000_000, 0, 
				{"test":(dataDir/"dataset-configs/crown_ttbar_testjob.json",
				dataDir/"workload-configs/crown_ttbar_testjob.json"),
				"copy":(dataDir/"dataset-configs/crown_ttbar_copyjob.json",
				dataDir/"workload-configs/crown_ttbar_copyjob_no_cpu.json")},
				data,loss,args.nocpu,args.networkratio)
			for j in range(int(math.log10(args.hyper_test_low)),
							int(math.log10(args.hyper_test_high))+1):
				stoptime=time.time()+3600
				print(0.01,10**j)
				calibrator.epsilon=10**j
				t0 = time.time()
				#cal=calibrator.calibrate(samplePoint, 3600, coordinator=coordinator)
				cal=calibrator.descend(simulator,eval(args.evaluate),stoptime)
				t1 = time.time()
				print(cal)
				print(t1-t0)
				if best is None or cal[1]<bestLoss:
					bestLoss=cal[1]
					best=(0.01,10**j)
			print(best,bestLoss)

	else:
		simulator = Simulator("dc-sim",dataDir/"platform-files/sgbatch_validation_template.xml", 
			[1.0,0.9,0.8,0.7,0.6,0.5,0.4,0.3,0.2,0.1,0.0], 10_000_000_000, 0, 
			{"test":(dataDir/"dataset-configs/crown_ttbar_testjob.json",
			dataDir/"workload-configs/crown_ttbar_testjob.json"),
			"copy":(dataDir/"dataset-configs/crown_ttbar_copyjob.json",
			dataDir/"workload-configs/crown_ttbar_copyjob_no_cpu.json")},
			data,loss,args.nocpu,args.networkratio)	
	
		t0 = time.time()
		cal=calibrator.calibrate(simulator, timelimit=args.timelimit, coordinator=coordinator)
		t1 = time.time()
		print ("We should now be printing the calibration")
		print(cal)
		print(t1-t0)

	