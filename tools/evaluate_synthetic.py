#!/usr/bin/env python3
import argparse
import sys
from glob import glob
def relative_error(real,other):
	return abs(real-other)/real
def main(args):
	files = []
	
	for pattern in args.files:
		if '*' in pattern or '?' in pattern:
			files += glob(pattern)
		else:
			files.append(pattern)
	args=eval(args.args)
	for file in files:
		with open(file, "r") as f:
			lines = f.readlines()
			mark=False
			for line in lines:
				if mark:
				
					cal=eval(line)
					#print(cal)
					error=0
					if "externalNetwork" in cal[0]:
						cal[0]["externalFastNetwork"]=cal[0]["externalNetwork"]*10
						cal[0]["externalSlowNetwork"]=cal[0]["externalNetwork"]
						del cal[0]["externalNetwork"]
					for key in cal[0]:
						if key == "xrootd_flops":
							continue
						error+=relative_error(args[key],cal[0][key])
					print(error,cal[1],file)
					break
				if "We should now be printing the calibration" in line:
					mark=True
					
if __name__=="__main__":

	parser = argparse.ArgumentParser(description="evaluate the synthetic err files")
	parser.add_argument("-f", "--files",nargs='+', type=str, required=True, help="files to use")
	parser.add_argument("-a", "--args", type=str, required=True, help="args to gen synthetic data for")
	args = parser.parse_args()
	main(args)