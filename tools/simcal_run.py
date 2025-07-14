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
import ddks#pip install git+https://github.com/pnnl/DDKS 
import torch #pip install torchvision
import matplotlib.pyplot as plt
#from pytorch3d.loss import chamfer_distance#pip install "git+https://github.com/facebookresearch/pytorch3d.git@stable"
#conda install pytorch3d -c pytorch3d
from scipy.spatial.distance import directed_hausdorff
import ot #pip install POT
from sklearn.metrics import mean_squared_error, mean_absolute_error
from simcal_calibrator import *

import time    
from skywalker import processify #pip install skywalker

toolsDir = Path(os.path.dirname(os.path.realpath(__file__)))  
# Get path to THIS folder where the simulator lives

if __name__=="__main__":

	parser = argparse.ArgumentParser(description="Calibrate DCSim using simcal")
	parser.add_argument("-g", "--groundtruth", type=str, required=True, help="Ground Truth data folder")
	parser.add_argument("-l", "--loss", type=str, required=True, help="Ground Truth data folder", default = "ddks")
	parser.add_argument('--nocpu', action='store_true', help="Dont calibrate CPU, instead use 1960Mf" )
	parser.add_argument("-r", "--networkratio", type=float, help="The ratio between slow and fast external network")
	parser.add_argument("-a", "--args", type=str, help="args to shell about")
	args = parser.parse_args()
	evaluator=sc.evaluation.LossCloud()
	if args.loss=="mre":
		loss=MRELoss
	elif args.loss=="ddks":
		loss=ddksLoss
	elif args.loss=="ratio":
		loss=MRELossRatio
	elif args.loss== "chamfer":
		loss=chamferLoss
	elif args.loss== "hausdorff":
		loss=hausdorffLoss
	elif args.loss== "wasserstein":
		loss=wassersteinLoss
	elif args.loss== "sorted":
		loss=sortedMRELoss
	elif args.loss== "double":
		loss=doubleSortedMRELoss
	else:
		print("unrecognized loss function",args.loss)
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
	

	#evaluator = sc.calibrators.Debug(sys.stdout)
	#evaluator = sc.calibrators.Grid()
	#evaluator = sc.calibrators.Random()(0.01, 0.001) 0.9656790133317311
	if not args.nocpu:
		evaluator.add_param("cpuSpeed", sc.parameter.Exponential(20, 40).format("%.2f"))
	evaluator.add_param("ramDisk", sc.parameter.Exponential(20, 40).format("%.2f"))
	evaluator.add_param("disk", sc.parameter.Exponential(20, 33).format("%.2f"))
	evaluator.add_param("internalNetwork", sc.parameter.Exponential(20, 33).format("%.2f"))
	evaluator.add_param("xrootd_flops", sc.parameter.Exponential(20, 47).format("%.2f"))
	
	if args.networkratio:
		evaluator.add_param("externalNetwork", sc.parameter.Exponential(20, 33).format("%.2f"))
	else:
		evaluator.add_param("externalFastNetwork", sc.parameter.Exponential(20, 33).format("%.2f"))
		evaluator.add_param("externalSlowNetwork", sc.parameter.Exponential(20, 33).format("%.2f"))

	dataDir=toolsDir/"../data"

	 
	
	simulator = Simulator("dc-sim",dataDir/"platform-files/sgbatch_validation_template.xml", 
		[1.0,0.9,0.8,0.7,0.6,0.5,0.4,0.3,0.2,0.1,0.0], 10_000_000_000, 0, 
		{"test":(dataDir/"dataset-configs/crown_ttbar_testjob.json",
		dataDir/"workload-configs/crown_ttbar_testjob.json"),
		"copy":(dataDir/"dataset-configs/crown_ttbar_copyjob.json",
		dataDir/"workload-configs/crown_ttbar_copyjob_no_cpu.json")},
		data,loss,args.nocpu,args.networkratio)	
	

	t0 = time.time()
	
	cal=simulator(eval(args.args))
	t1 = time.time()
	print ("Finished")
	print(cal)
	print(t1-t0)

	