#!/usr/bin/env python3
from simcal_calibrator import *
from time import time
if __name__=="__main__":

	parser = argparse.ArgumentParser(description="Calibrate DCSim using simcal")
	parser.add_argument("-g", "--groundtruth", type=str, required=True, help="Ground Truth data folder")
	#parser.add_argument("-t", "--timelimit", type=int, required=True, help="Timelimit in seconds")
	#parser.add_argument("-c", "--cores", type=int, required=True, help="Number of CPU cores")
	args = parser.parse_args()
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
		   
	simulator = Simulator("dc-sim")
	#calibrator = sc.calibrators.Debug(sys.stdout)
	#calibrator = sc.calibrators.Grid()
	#calibrator = sc.calibrators.Random()
	best=None
	bestLoss=None
	for i in range(10):
		for j in range(10):
			stoptime=time()+3600
			calibrator = sc.calibrators.GradientDescent(1/10**i,1/10**j,early_reject_loss=1.0)
			calibrator.add_param("cpuSpeed", sc.parameter.Exponential(20, 40).format("%.2f"))
			calibrator.add_param("ramDisk", sc.parameter.Exponential(20, 40).format("%.2f"))
			calibrator.add_param("disk", sc.parameter.Exponential(20, 33).format("%.2f"))
			calibrator.add_param("internalNetwork", sc.parameter.Exponential(20, 33).format("%.2f"))
			calibrator.add_param("externalFastNetwork", sc.parameter.Exponential(20, 33).format("%.2f"))
			calibrator.add_param("externalSlowNetwork", sc.parameter.Exponential(20, 33).format("%.2f"))

			dataDir=toolsDir/"../data"
			samplePoint = SamplePoint(simulator,dataDir/"platform-files/sgbatch_validation_template.xml", [1.0,0.9,0.8,0.7,0.6,0.5,0.4,0.3,0.2,0.1,0.0], 10_000_000_000, 0, {"test":(dataDir/"dataset-configs/crown_ttbar_testjob.json",dataDir/"workload-configs/crown_ttbar_testjob.json"),"copy":(dataDir/"dataset-configs/crown_ttbar_copyjob.json",dataDir/"workload-configs/crown_ttbar_copyjob.json")},data)
			coordinator = sc.coordinators.ThreadPool(pool_size=args.cores) 
			#maxs=samplePoint(	{"cpuSpeed":"1970Mf",	"disk":"17MBps", "ramDisk":"1GBps",	"internalNetwork":"10GBps",	"externalSlowNetwork":"1.15Gbps", "externalFastNetwork":"11.5Gbps"})
			#print("Max's",maxs)
			t0 = time()
			cal=calibrator.calibrate(samplePoint, timelimit=args.timelimit, coordinator=coordinator)
			cal=calibrator.descend(samplePoint,{'cpuSpeed': 1959376102.6873443, 'ramDisk': 7960898256.745884, 'disk': 13264014.962423073, 'internalNetwork': 7958060335.116626, 'externalFastNetwork': 4258718314.2515693, 'externalSlowNetwork': 5966513.835689467},stoptime)
			t1 = time()
			#print ("We should now be printing the calibration")
			print(cal)
			print(t1-t0)
			if best is None or cal[1]<bestLoss:
				bestLoss=cal[1]
				best=(1/10**i,1/10**j)
	print(best,bestLoss)

	