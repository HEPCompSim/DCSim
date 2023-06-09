#! /usr/bin/python3
#./gradientSearch.py -r ../../DCSIM\ calibration\ Data/individualSlowRawData.json -p ../data/platform-files/sgbatch_validation_template.xml -t 60 -hr 1,.5,0 -s 29 32 -rb 23 25 -ilb 29 32 -rd 29 32 --type dynamic -d 0.01 -e .0001 --seed 1

import concurrent.futures
import multiprocessing
import argparse
from math import *
from oneTest import oneEval, initEvaluator
from itertools import product
from fractions import Fraction
import threading
import time
import random
from multiprocessing.managers import SyncManager

def sampleHyperplane(x,ref,mag,vector,stepSize):
	s=mag
	for i in range(len(x)):
		s+=(x[i]-ref[i])*vector[i]/stepSize
	return s
class ObjectManager(SyncManager):
    pass

startTime = time.time()
extractedResults=[]

class GradMethod:
	def __init__(this,stop_signal, i, hitrates,args):
		this.results=[]
		this.count=0
		this.stop_signal=stop_signal
		this.id=i
		this.hitrates=hitrates
		this.xblock=args.xblock
		this.nblock=args.nblock
		this.args=args
		this.retArgs=None
		this.retResult=None
	def normalize(array,mag=1):
		s=sum(array)/mag
		for i in range(len(array)):
			array[i]/=s
		return array
	def descend(this):
		pass
	def internalEval(this, val,runType):
		result=evaluate_combination(this.stop_signal,args, val, this.id, this.hitrates,this.xblock,this.nblock,runType)
		this.short(result)
		this.count+=1
		return result
	def short(this,result):
		if result is None:
			raise (GeneratorExit)
		elif(len(result)==3):	
			this.results.append(result[2])
		
		if  this.stop_signal.is_set():
			raise (GeneratorExit)
	def collect(this):
		return this.retResult, this.retArgs, this.results, this.count
class DynamicGrad(GradMethod):
	def __init__(this,stop_signal,i,hitrates,args,initialPoint):
		super().__init__(stop_signal,i,hitrates,args)
		this.ranges=(args.speed,args.read_bandwidth, args.internal_link_bandwidth, args.remote_bandwidth)
		this.vals=list(initialPoint)
		this.eps=args.epsilon
		this.delta=args.delta
		try:
			res=this.internalEval(initialPoint,"dynamicSearch random")
			this.currentResult=list(res)
			this.retArgs=res[1]
			this.retResult=res[0]
		except(GeneratorExit):
			pass 
		
	def descend(this):
		try:
			vector=[]
			best=this.currentResult[0]
			bestArgs=list(this.currentResult[1])
			
			for i in range(len(this.vals)):
				tvals=this.vals.copy()
				tvals[i]+=this.eps
				result=this.internalEval(tvals,"dynamicSearch gradient")
				if(result[0]<best):
					best=result[0]
					bestArgs=result[1]
				vector.append((result[0]-this.currentResult[0])/this.currentResult[0])
			
			
			
			trial=10*this.eps
			while True:
				eVal=this.vals.copy()
				
				for i in range(len(this.vals)):
					eVal[i]+=vector[i]
				expected=sampleHyperplane(eVal,this.currentResult[1],this.currentResult[0],vector,this.eps)
				result=this.internalEval(eVal,"dynamicSearch line")
				
				
				if(result[0]<best):
					best=result[0]
					bestArgs=result[1]
				if(result[0]<=expected):
					this.eps=trial
					break#this will happen eventually, when trial approaches 0
				else:
					trial/=2
			improvement=(abs(best-this.currentResult[0])/this.currentResult[0])
			
			this.currentResult[0]=best
			this.currentResult[1]=tuple(bestArgs)
			if improvement<args.delta:
				return True
			return False
		except(GeneratorExit):
			return True
class FiniteGrad(GradMethod):
	def __init__(this,stop_signal,i,hitrates,args,initialPoint):
		super().__init__(stop_signal,i,hitrates,args)
		this.ranges=(args.speed,args.read_bandwidth, args.internal_link_bandwidth, args.remote_bandwidth)
		this.vals=list(initialPoint)
		this.eps=args.epsilon
		
		this.eps2=args.epsilon
		this.delta=args.delta
		try:
			res=this.internalEval(initialPoint,"finiteSearch random")
			this.currentResult=list(res)
			this.retArgs=res[1]
			this.retResult=res[0]
		except(GeneratorExit):
			pass 
		
	def descend(this):
		try:
			vector=[]
			best=this.currentResult[0]
			bestArgs=list(this.currentResult[1])
			
			for i in range(len(this.vals)):
				tvals=this.vals.copy()
				tvals[i]+=this.eps2
				result=this.internalEval(tvals,"finiteSearch gradient")
				if(result[0]<best):
					best=result[0]
					bestArgs=result[1]
				vector.append((result[0]-this.currentResult[0])/this.currentResult[0])
			
			
			
			trial=10*this.eps
			while True:
				eVal=this.vals.copy()
				
				for i in range(len(this.vals)):
					eVal[i]+=vector[i]
				expected=sampleHyperplane(eVal,this.currentResult[1],this.currentResult[0],vector,this.eps)
				result=this.internalEval(eVal,"finiteSearch line")
				
				
				if(result[0]<best):
					best=result[0]
					bestArgs=result[1]
				if(result[0]<=expected):
					this.eps=trial
					break#this will happen eventually, when trial approaches 0
				else:
					trial/=2
			improvement=(abs(best-this.currentResult[0])/this.currentResult[0])
			
			this.currentResult[0]=best
			this.currentResult[1]=tuple(bestArgs)
			if improvement<args.delta:
				return True
			return False
		except(GeneratorExit):
			return True
def valid_type(value):
	value=value.lower()
	valid_inputs = ['finite', 'limit', 'dynamic']
	if value in valid_inputs:
		return value
	for input in valid_inputs:
		if input.startswith(value):
			return input
	raise argparse.ArgumentTypeError(f"Invalid input: {value}")
		
parser = argparse.ArgumentParser(description='Gradient Descent in a logimetric paramiter space.\nNote:Max/min paramiters control the bounds of the initial starting points and the relative "scale" of the axes, but do not limit where the search can travel.')
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
parser.add_argument('-e', '--epsilon', type=float, help='The initial learning rate of the gradient search', required=True)
parser.add_argument('-e2', '--epsilon2', type=float, help="The size of the finite difference.  Only used by type 'Finite'")
parser.add_argument('-d', '--delta', type=float, help="Each gradient stops early if the percentage difference between consecutive gradient itterations is less than delta.", required=True)
parser.add_argument('--seed', type=int, help="Seed for initial points to enable fair comparison between gradient search methods'")
parser.add_argument( '--type', type=valid_type, help='Gradient search type.  \n\t\'Limit\' INCOMPLETE takes the limit of multiple simulations per gradient to find the truest gradient.\n\t \'Finite\' approximates the Gradient by taking 1 sample in each dimension a fixed delta out, and then moving down the gradient then moving epsilon down it.\n\t\'Dynamic\' approximates the Gradient by taking 1 sample in each dimension an epsilon out, then moving down the resulting vector.  As move distance is reduced, so is the sample radius.', required=True)

import re
args = parser.parse_args()
if args.type=='finite' and not args.epsilon2:  
	sys.err.print("Epsilon 2 must be specified for finite")
	sys.exit()
hitrates=re.split(',|\s|;',args.hitrates)


#print(args)
best=None
minV=0
def interpolate(x,minV,maxV):
	if minV>maxV:
		minV,maxV=maxV,minV
	
	return x*(maxV-minV)+minV
def evaluate_combination(stop_signal,args, val, i, hitrates,xblock,nblock,runtype):
	if not stop_signal.is_set():
		speedI,readI,inBandI, reBandI = val
		speed = pow(2, interpolate(speedI, args.speed[0], args.speed[1]))
		read = pow(2, interpolate(readI, args.read_bandwidth[0], args.read_bandwidth[1]))
		inBand = pow(2, interpolate(inBandI, args.internal_link_bandwidth[0], args.internal_link_bandwidth[1]))
		reBand = pow(2, interpolate(reBandI, args.remote_bandwidth[0], args.remote_bandwidth[1]))
		#print('Running %.2E %.2E %.2E %.2E:' % (speed, read, inBand, reBand))
		v,results = oneEval(args.platform, speed, read, inBand, reBand, hitrates,xblock,nblock,uniqueID=i,runtype=runtype)
		#print(v)
		return (v, (speed, read, inBand, reBand),results)
	else:
		return None

		
def search_thread(stop_signal, args,val, i, hitrates):
	if stop_signal.is_set():
		return None
	search=None
	if args.type=='dynamic':
		search=DynamicGrad(stop_signal,i,hitrates,args,val)
	while not stop_signal.is_set():
		if search.descend():
			break

	return search.collect()
def parallel_grad_search(args):
	initEvaluator(args.reference)
	dimensionality = 4

	best = None
	minV = 0
	with ObjectManager() as manager:
		stop_signal = manager.Event()
		signal_timer = threading.Timer(args.time, stop_signal.set)
		signal_timer.start()
		random.seed(args.seed)
				

		with concurrent.futures.ProcessPoolExecutor() as executor:
			results = []
			while not stop_signal.is_set():
				results = []
				i = 0
				
				
				for i in range(multiprocessing.cpu_count()*100):
					val=(random.random(),random.random(),random.random(),random.random())
					result = executor.submit(search_thread, stop_signal,args, val, i, hitrates)
					results.append(result)

				best = None
				minV = None
				count=0
				exCount=0
				for resultSet in results:
					global extractedResults
					if resultSet.cancelled():
						continue
					resultSet=resultSet.result()
					if resultSet is None:
						continue
					v, combination,allResults,subcount= resultSet
					count+=1
					exCount+=subcount
					if v is None:
						print(resultSet)
					if best is None or minV is None:
							minV = v
							best = combination
					elif v < minV:
						minV = v
						best = combination
					for thingToAppend in allResults:
						extractedResults+=thingToAppend
						
						
							# print("New Best!")
	print(str(count)+" gradients followed sampling "+str(exCount)+" points")
	print("Best " + str(minV) + " " + str(best))


# Run the parallel grad search
try:
	parallel_grad_search(args)
	with open(args.type+"GradientSearchResults.txt", 'a') as writer:
		for result in extractedResults:
			writer.write(str(result)+"\n")
except KeyboardInterrupt:
	pass
#weird theory question: more effienct compiled lookup table
#Follow up, bidirectional map
