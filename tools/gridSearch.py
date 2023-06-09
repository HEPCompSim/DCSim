#! /usr/bin/python3
#./gridSearch.py -r ../../DCSIM\ calibration\ Data/individualSlowRawData.json -p ../data/platform-files/sgbatch_validation_template.xml -n 1000 -hr 1,.5,0 -s 29 32 -rb 23 25 -ilb 29 32 -rd 29 32
import concurrent.futures
import multiprocessing
import argparse
from math import *
from oneTest import oneEval, initEvaluator
from itertools import product
from fractions import Fraction
import time
startTime = time.time()
extractedResults=[]
def gridKey(a):
	at=0
	for i in a:
		at+=smallest_denominator(i)
	return at
def smallest_denominator(decimal):
	fraction = Fraction(decimal).limit_denominator()
	return fraction.denominator
def interpolate(x,minV,maxV):
	if minV>maxV:
		minV,maxV=maxV,minV
	
	return x*(maxV-minV)+minV
class gridItterator:
	def __init__(self,dims):
		self.dims=dims
	def __iter__(self):
		return self
	def __next__(self):
		denum=1
		core=[0,1]
		currentSet={1,0}
			
		while True:
			for i in sorted(product(core,repeat=self.dims),reverse=True,key=gridKey):
				for j in i:
					if j in currentSet:#prevent repeats by requiring atleast 1 element of the touple to be from the current set of numbers
						#print(i)
						yield i
						break
						
			currentSet.clear()
			denum*=2
			update=[i/denum for i in range(1,denum,2)]
			currentSet.update(update)
			update+=core
			core=update				
parser = argparse.ArgumentParser(description='Grid Search in a logimetric grid.')
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
best=None
minV=0
def evaluate_combination(args, val, i, hitrates,xblock,nblock):
	
	speedI,readI,inBandI, reBandI = val
	speed = pow(2, interpolate(speedI, args.speed[0], args.speed[1]))
	read = pow(2, interpolate(readI, args.read_bandwidth[0], args.read_bandwidth[1]))
	inBand = pow(2, interpolate(inBandI, args.internal_link_bandwidth[0], args.internal_link_bandwidth[1]))
	reBand = pow(2, interpolate(reBandI, args.remote_bandwidth[0], args.remote_bandwidth[1]))
	#print('Running %.2E %.2E %.2E %.2E:' % (speed, read, inBand, reBand))
	v,results = oneEval(args.platform, speed, read, inBand, reBand, hitrates,xblock,nblock,uniqueID=i,runtype="grid")
	#print(v)
	return (v, (speed, read, inBand, reBand),results,time.time())

def parallel_grid_search(args):
	initEvaluator(args.reference)
	dimensionality = 4

	best = None
	minV = 0

	
			

	with concurrent.futures.ProcessPoolExecutor() as executor:
		results = []
		i = 0
		ittr = iter(gridItterator(dimensionality))
		ittr = next(ittr)
		
		while time.time() - startTime < args.time:
			i += 1
			val = next(ittr)
			result = executor.submit(evaluate_combination, args, val, i, hitrates, args.xblock, args.nblock)
			results.append(result)
		
		# Cancel remaining tasks
		for result in results:
			result.cancel()

		best = None
		minV = None
		count=0
		for result in results:
			global extractedResults
			if result.cancelled():
				continue
			
			v, combination,allResults,time = result.result()
			extractedResults+=allResults
			if time > startTime+args.time:
				continue#discard overtime simulation
			count+=1	
			if best is None:
				minV = v
				best = combination
			elif v < minV:
				minV = v
				best = combination
				# print("New Best!")
	print(str(count)+" grid points sampled")
	print("Best " + str(minV) + " " + str(best))


# Run the parallel grid search
try:
	parallel_grid_search(args)
	with open("randomSearchResults.txt", 'a') as writer:
		for result in extractedResults:
			writer.write(str(result)+"\n")
except KeyboardInterrupt:
	pass
#weird theory question: more effienct compiled lookup table
#Follow up, bidirectional map
