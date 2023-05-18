#! /usr/bin/python3
import argparse
from math import *
from oneTest import oneEval, initEvaluator
interpolations={}
def interpolate(x,minV,maxV,c):
	if minV>maxV:
		minV,maxV=maxV,minV
	if (minV,maxV) in interpolations:
		a=interpolations[(minV,maxV)]
	else:
		return x*(maxV-minV)/c+minV
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

initEvaluator(args.reference)
dimensionality=4
dimDiv=int(pow(args.sims,1/dimensionality))
ittrC=int(pow(dimDiv,dimensionality))
print("Running Grid search with "+str(dimDiv)+" samples per dimension for a total of "+str(ittrC)+" test:")
#print(args)
best=None
minV=0
try:
	for i in range(ittrC):#I figured it would be better to itterate this multidimensional space as a single loop and then unpack it to each requisit variable
		carry=i
		speedI=carry%dimDiv
		carry//=dimDiv
		readI=carry%dimDiv
		carry//=dimDiv
		inBandI=carry%dimDiv
		carry//=dimDiv
		reBandI=carry%dimDiv
		speed=pow(2,interpolate(speedI,args.speed[0],args.speed[1],dimDiv))
		read=pow(2,interpolate(readI,args.speed[0],args.read_bandwidth[1],dimDiv))
		inBand=pow(2,interpolate(inBandI,args.speed[0],args.internal_link_bandwidth[1],dimDiv))
		reBand=pow(2,interpolate(reBandI,args.speed[0],args.remote_bandwidth[1],dimDiv))
		
		print('Running %.2E %.2E %.2E %.2E:' % (speed, read, inBand, reBand),end=' ')
		v=oneEval(args.platform,speed, read, inBand, reBand,hitrates)
		print(v)
		if best is None:
			minV=v
			best=(speed, read, inBand, reBand)
		elif v<minV:
			minV=v
			best=(speed, read, inBand, reBand)
			print("New Best!")
	print("Best "+str(minV)+" "+str(best))
except KeyboardInterrupt:
	pass
#weird theory question: more effienct compiled lookup table
#Follow up, bidirectional map
