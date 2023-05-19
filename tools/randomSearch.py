#! /usr/bin/python3
#./randomSearch.py -r ../../DCSIM\ calibration\ Data/individualSlowRawData.json -p ../data/platform-files/sgbatch_validation_template.xml -n 1000 -hr 1,.5,0 -s 29 32 -rb 23 25 -ilb 29 32 -rd 29 32

import multiprocessing
import argparse
from math import *
from oneTest import oneEval, initEvaluator
import random
def randomSample(minV,maxV):
	if minV>maxV:
		minV,maxV=maxV,minV
	return random.uniform(minV,maxV)
parser = argparse.ArgumentParser(description='Grid Search in a logimetric grid.')
parser.add_argument('-r', '--reference', type=str, help='Reference values file path', required=True)
parser.add_argument('-p', '--platform', type=str, help='Template Platform file path', required=True)
parser.add_argument('-n', '--sims', type=float, help='the maximum number of times the hitrate scan script should be ran.', required=True)
parser.add_argument('-hr', '--hitrates', type=str, help='python array of hitrates to use', required=True)

parser.add_argument('-s', '--speed', nargs=2,type=float, help='compute speed option with 3 values range',metavar=('min', 'max'), required=True)
parser.add_argument('-rb', '--read-bandwidth', nargs=2,type=float, help='host read bandwidth range',metavar=('min', 'max'), required=True)

parser.add_argument('-ilb', '--internal-link-bandwidth', nargs=2,type=float, help='internal link bandwidth range',metavar=('min', 'max'), required=True)
parser.add_argument('-rd', '--remote-bandwidth', nargs=2,type=float, help='remote bandwidth option range',metavar=('min', 'max'), required=True)
import re
args = parser.parse_args()
hitrates=re.split(',|\s|;',args.hitrates)

#print(args)

def randomItteration(args, dimDiv, i, hitrates):
	speed = pow(2, randomSample(args.speed[0], args.speed[1]))
	read = pow(2, randomSample(args.read_bandwidth[0], args.read_bandwidth[1]))
	inBand = pow(2, randomSample(args.internal_link_bandwidth[0], args.internal_link_bandwidth[1]))
	reBand = pow(2, randomSample(args.remote_bandwidth[0], args.remote_bandwidth[1]))

	#print('Running %.2E %.2E %.2E %.2E:' % (speed, read, inBand, reBand), end=' ')
	v = oneEval(args.platform, speed, read, inBand, reBand, hitrates,uniqueID=i)
	#print(v)
	return (v, (speed, read, inBand, reBand))

def parallel_random_search(args):
	initEvaluator(args.reference)
	dimensionality = 4
	dimDiv = int(pow(args.sims, 1/dimensionality))
	ittrC = int(pow(dimDiv, dimensionality))

	print("Running Grid search with "+str(ittrC)+" test:")
	best = None
	minV = 0


	pool = multiprocessing.Pool()
	results = []
	for i in range(ittrC):
		result = pool.apply_async(randomItteration, (args, dimDiv, i, hitrates))
		results.append(result)

	for result in results:
		v, combination = result.get()
		if best is None:
		    minV = v
		    best = combination
		elif v < minV:
		    minV = v
		    best = combination
		    #print("New Best!")

	pool.close()
	pool.join()

	print("Best " + str(minV) + " " + str(best))


# Run the parallel grid search
try:
	parallel_random_search(args)
except KeyboardInterrupt:
    pass
#weird theory question: more effienct compiled lookup table
#Follow up, bidirectional map
