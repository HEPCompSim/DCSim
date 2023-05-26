#! /usr/bin/python3
from platformFromVector import pFromV
from sumarize_sim import extract
import sys
import json
import shutil
import subprocess
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
		count=1
	return ret/count
def oneTest(xml_file_path, cpu_speed, read_speed, link_speed, net_speed,hitrates,xblock,nblock,uniqueID=None,runtype=None):
	hits=' '.join([str(float(i)) for i in hitrates])
	platform=pFromV(xml_file_path, cpu_speed, read_speed, link_speed, net_speed)
	if(uniqueID):
	
		process = subprocess.run(["./hitrateScanScript.sh",platform,hits,str(uniqueID),str(xblock),str(nblock)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
		ret=extract("../tmp/outputs/"+str(uniqueID),{"cpu_speed":cpu_speed,"read_speed":read_speed,"link_speed":link_speed,"net_speed":net_speed,"xblock":xblock,"nblock":nblock,"run_type":runtype})
		shutil.rmtree("../tmp/outputs/"+str(uniqueID), ignore_errors=True)
		return ret
	else:
		process = subprocess.run(["./hitrateScanScript.sh",platform,hits,str(uniqueID),str(xblock),str(nblock)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
		return extract("../tmp/outputs",{"cpu_speed":cpu_speed,"read_speed":read_speed,"link_speed":link_speed,"net_speed":net_speed,"xblock":xblock,"nblock":nblock,"run_type":runtype})
def oneEval(xml_file_path, cpu_speed, read_speed, link_speed, net_speed,hitrates,xblock,nblock,uniqueID=None,rff_run=None,runtype=None):
	run,allResults=oneTest(xml_file_path, cpu_speed, read_speed, link_speed, net_speed,hitrates,xblock,nblock,uniqueID,runtype)
	return evaluate(run,rff_run),allResults
def main():
	rff_run,xml_file_path, cpu_speed, read_speed, link_speed, net_speed = sys.argv[1:]
	initEvaluator(rff_run)
	print(oneEval(xml_file_path, cpu_speed, read_speed, link_speed, net_speed,list(range(10))))
if __name__ == '__main__':
	main()
