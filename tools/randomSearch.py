#! /usr/bin/python3
#./randomSearch.py -r ../../DCSIM\ calibration\ Data/individualSlowRawData.json -p ../data/platform-files/sgbatch_validation_template.xml -t 60 -hr 1,.5,0 -s 29 32 -rb 23 25 -ilb 29 32 -rd 29 32

import multiprocessing
import argparse
from math import *
from oneTest import oneEval, initEvaluator
import random
import time
import concurrent.futures
from random import SystemRandom
startTime = time.time()
def randomSample(minV,maxV,generator):
	if minV>maxV:
		minV,maxV=maxV,minV
	return generator.uniform(minV,maxV)
parser = argparse.ArgumentParser(description='random Search in a logimetric grid.')
parser.add_argument('-r', '--reference', type=str, help='Reference values file path', required=True)
parser.add_argument('-p', '--platform', type=str, help='Template Platform file path', required=True)
parser.add_argument('-t', '--time', type=int, help='the time budget (in seconds) to run', required=True)
parser.add_argument('-hr', '--hitrates', type=str, help='python array of hitrates to use', required=True)
parser.add_argument('-xb', '--xblock', type=float, help='The xrootd blocksize',default=1000000000)
parser.add_argument('-nb', '--nblock', type=float, help='the network blocksize',default=100000000)

parser.add_argument('-s', '--speed', nargs=2,type=float, help='compute speed option with 3 values range',metavar=('min', 'max'), required=True)
parser.add_argument('-rb', '--read-bandwidth', nargs=2,type=float, help='host read bandwidth range',metavar=('min', 'max'), required=True)

parser.add_argument('-ilb', '--internal-link-bandwidth', nargs=2,type=float, help='internal link bandwidth range',metavar=('min', 'max'), required=True)
parser.add_argument('-rd', '--remote-bandwidth', nargs=2,type=float, help='remote bandwidth option range',metavar=('min', 'max'), required=True)
import re
args = parser.parse_args()
hitrates=re.split(',|\s|;',args.hitrates)

#print(args)
extractedResults=[]
def randomItteration(args, i, hitrates,xblock,nblock,generator):
	speed = pow(2, randomSample(args.speed[0], args.speed[1],generator))
	read = pow(2, randomSample(args.read_bandwidth[0], args.read_bandwidth[1],generator))
	inBand = pow(2, randomSample(args.internal_link_bandwidth[0], args.internal_link_bandwidth[1],generator))
	reBand = pow(2, randomSample(args.remote_bandwidth[0], args.remote_bandwidth[1],generator))

	#print('Running %.2E %.2E %.2E %.2E:' % (speed, read, inBand, reBand), end=' ')
	v,allResults= oneEval(args.platform, speed, read, inBand, reBand, hitrates,xblock,nblock,uniqueID=i,runtype="random")
	#print(v)
	return v, (speed, read, inBand, reBand),allResults,time.time()

def parallel_random_search(args):
	initEvaluator(args.reference)
	dimensionality = 4
	best = None
	minV = None
	count=0
	with concurrent.futures.ProcessPoolExecutor() as executor:
		results = []
		i = 0
		ongoing=True
		while ongoing:
			generator=random.SystemRandom()
			for iii in range(multiprocessing.cpu_count()*100):
				i += 1
				result = executor.submit(randomItteration, args, i, hitrates, args.xblock, args.nblock,generator)
				results.append(result)
			
			if time.time() - startTime > args.time:
				for result in results:
					result.cancel()
				ongoing=False

			
			for result in results:
				global extractedResults
				if result.cancelled():
					continue
				
				v, combination,allResults,timeV = result.result()
				extractedResults+=allResults
				if timeV > startTime+args.time:
					continue#discard overtime simulation
				count+=1
				if best is None:
					minV = v
					best = combination
				elif v < minV:
					minV = v
					best = combination
					print(str(time.time()-startTime)+" New Best " + str(minV) + " " + str(best))
					print(str(count)+" grid points sampled")

			with open("randomSearchResults.txt", 'a') as writer:
				extractedResultsB=extractedResults
				extractedResults=[]
				for result in extractedResultsB:
					writer.write(str(result)+"\n")
	print("Final Best " + str(minV) + " " + str(best))
	print(str(count)+" grid points sampled")

# Run the parallel grid search
try:
	parallel_random_search(args)

	#print(extractedResults)
except KeyboardInterrupt:
    pass
#weird theory question: more effienct compiled lookup table
#Follow up, bidirectional map
