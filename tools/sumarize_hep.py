#! /usr/bin/python3
#this script 85% written by ChatGPT, I love this technology!
import argparse
import json
import os
import glob
import statistics
import csv

parser = argparse.ArgumentParser(description='Process some data files.')
parser.add_argument('datadir', metavar='datadir', type=str, help='the directory containing data files')
parser.add_argument('-o', '--output', type=str, help='the output file name')
parser.add_argument('--csv', action='store_true', help='output as CSV instead of JSON')
args = parser.parse_args()

cacheSaturated = {'individualJobs': {}, 'duplicatedJobs': {}}

for jobType in ['individualJobs', 'duplicatedJobs']:
	for speed in [1, 10]:
		cacheSaturated[jobType][str(speed)+"gbps"] = {}
		for hitrate in [round(x * 0.1, 1) for x in range(0, 11)]:
			cacheSaturated[jobType][str(speed)+"gbps"]["hitrate"+str(hitrate)] = {}
			hddcache='_HDDcache' if jobType=='individualJobs' else ''
			hitrateDir = os.path.join(args.datadir, f"{jobType}/*_Gateway{speed}Gbps"+hddcache+"*/clusterid_*_hitrate_"+str(hitrate)+".csv")
			#print(hitrateDir)
			for datafile in glob.glob(hitrateDir):
				#print(datafile)
				if "bug_" in datafile or "Corrupted" in datafile or "copyjobs" in datafile or "50Mcache" in datafile or "onlyoneinputfile" in datafile or (not hddcache and "_HDDcache" in datafile):
					continue
				with open(datafile) as f:
					reader = csv.DictReader(f, delimiter=',')
					reader.fieldnames = [field.strip() for field in reader.fieldnames]
					for row in reader:
						machine = row['machine.name'].strip()
						runtime = float(row['job.end'])-float(row['job.start'])
						if machine not in cacheSaturated[jobType][str(speed)+"gbps"]["hitrate"+str(hitrate)]:
							cacheSaturated[jobType][str(speed)+"gbps"]["hitrate"+str(hitrate)][machine] = {'runtimes': [], 'average': None, 'stdev': None}
						cacheSaturated[jobType][str(speed)+"gbps"]["hitrate"+str(hitrate)][machine]['runtimes'].append(runtime)
			for machine in cacheSaturated[jobType][str(speed)+"gbps"]["hitrate"+str(hitrate)]:
				runtimes = cacheSaturated[jobType][str(speed)+"gbps"]["hitrate"+str(hitrate)][machine]['runtimes']
				if runtimes:
					average = sum(runtimes) / len(runtimes)
					stdev = statistics.stdev(runtimes) if len(runtimes) > 1 else None
					cacheSaturated[jobType][str(speed)+"gbps"]["hitrate"+str(hitrate)][machine]['average'] = average
					cacheSaturated[jobType][str(speed)+"gbps"]["hitrate"+str(hitrate)][machine]['stdev'] = stdev
					cacheSaturated[jobType][str(speed)+"gbps"]["hitrate"+str(hitrate)][machine].pop('runtimes')

if args.csv:
	if args.output is None:
		args.output = '../tmp/hep-summary.csv'
	with open(args.output, 'w') as csvfile:
		writer = csv.writer(csvfile)
		writer.writerow(['Data Type', 'Speed', 'Hitrate', 'Machine', 'Average runtime', 'Standard deviation'])
		for dataType in ["individualJobs", "duplicatedJobs"]:
			for speed in cacheSaturated[dataType]:
				for hitrate in cacheSaturated[dataType][speed]:
					for machine in cacheSaturated[dataType][speed][hitrate]:
						avg = cacheSaturated[dataType][speed][hitrate][machine]['average']
						stdev = cacheSaturated[dataType][speed][hitrate][machine]['stdev']
						writer.writerow([dataType, speed, hitrate, machine, avg, stdev])
else:	

	if args.output is None:
		args.output = '../tmp/hep-summary.json'
	output_dict = {
		"individualJobs": {},
		"duplicatedJobs": {}
	}
	for dataType in ["individualJobs", "duplicatedJobs"]:
		for speed in cacheSaturated[dataType]:
			output_dict[dataType][speed] = {}
			for hitrate in cacheSaturated[dataType][speed]:
				output_dict[dataType][speed][hitrate] = {}
				for machine in cacheSaturated[dataType][speed][hitrate]:
					data = cacheSaturated[dataType][speed][hitrate][machine]
					if 'runtimes' in data:
						data.pop('runtimes')
					output_dict[dataType][speed][hitrate][machine] = data
	with open(args.output, 'w') as f:
		json.dump(output_dict, f, indent=4)
	print("output written to '"+os.path.abspath(args.output)+"'")


