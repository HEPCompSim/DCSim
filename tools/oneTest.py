#! /usr/bin/python3
from platformFromVector import pFromV
from sumarize_sim import extract
import sys
import json
import shutil
import subprocess
import os
import time
debugMode =False
file_path = os.path.dirname(os.path.realpath(__file__))
refferenceRun=None
def initEvaluator(refferencePath):
	global refferenceRun
	with open(refferencePath,'r') as file:
		refferenceRun=json.load(file)
def evaluate(run,refference=None):
	if(refference is None):
		refference=refferenceRun
	ret=0
	count=0
	for hitrate in run:
		for machine in run[hitrate]:
			count+=1
			#L1 
			ret+=abs(
				float(run       [hitrate][machine]['average'])-
				float(refference[hitrate][machine]['average'])
			)
	if(count==0):
		return float('inf')
	return ret/count
def oneTest(xml_file_path, cpu_speed, read_speed, link_speed, net_speed,hitrates,xblock,nblock,uniqueID=None,runtype=None,remove=True,timeout=None):
	hits=' '.join([str(float(i)) for i in hitrates])
	platform=pFromV(xml_file_path, cpu_speed, read_speed, link_speed, net_speed)
	if( not uniqueID is None):
		uniqueID=str(os.getpid())+"_"+str(uniqueID)+"_outputs"
		runstart=time.time()
		if debugMode:
			process = subprocess.run([file_path+"/hitrateScanScript.sh", platform, hits, uniqueID, str(int(float(xblock))), str(int(float(nblock)))], stdout=subprocess.PIPE, stderr=subprocess.PIPE,timeout=timeout)
			if process.stderr:
				print(process.stdout.decode())
				print(process.stderr.decode())
				print((xml_file_path, cpu_speed, read_speed, link_speed, net_speed,hitrates,xblock,nblock,uniqueID,runtype))
				print([file_path+"/hitrateScanScript.sh", "platform", hits, uniqueID, str(xblock), str(nblock)])
		else:
			process = subprocess.run([file_path+"/hitrateScanScript.sh", platform, hits, uniqueID, str(int(float(xblock))), str(int(float(nblock)))], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,timeout=timeout)
		runend=time.time()
		ret=extract(uniqueID,{"cpu_speed":cpu_speed,"read_speed":read_speed,"link_speed":link_speed,"net_speed":net_speed,"xblock":xblock,"nblock":nblock,"run_type":runtype,"clock_time":(runend-runstart)})
		if(remove):
			shutil.rmtree(uniqueID, ignore_errors=True)
		return ret
		
	else:
		process = subprocess.run([file_path+"/hitrateScanScript.sh",platform,hits,str(uniqueID),str(xblock),str(nblock)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
		return extract(file_path+"/../tmp/outputs",{"cpu_speed":cpu_speed,"read_speed":read_speed,"link_speed":link_speed,"net_speed":net_speed,"xblock":xblock,"nblock":nblock,"run_type":runtype})
def oneEval(xml_file_path, cpu_speed, read_speed, link_speed, net_speed,hitrates,xblock,nblock,uniqueID=None,rff_run=None,runtype=None,remove=True,timeout=None):
	try:
		run,allResults=oneTest(xml_file_path, cpu_speed, read_speed, link_speed, net_speed,hitrates,xblock,nblock,uniqueID,runtype,remove,timeout)
	except subprocess.TimeoutExpired:
		return float('inf'),[] 
	return evaluate(run,rff_run),allResults
def main():
	rff_run,xml_file_path, cpu_speed, read_speed, link_speed, net_speed,xrootdBlock, nblock = sys.argv[1:]
	initEvaluator(rff_run)
	print(oneEval(xml_file_path, cpu_speed, read_speed, link_speed, net_speed,list([x/10 for x in range(11)]),xrootdBlock, nblock,1,None,None,False)[0])
if __name__ == '__main__':
	main()
