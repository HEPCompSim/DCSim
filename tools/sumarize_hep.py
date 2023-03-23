#! /usr/bin/python3
#this script 90% written by ChatGPT, I love this technology!
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

cacheSaturated = {}
for speed in [1, 10]:
	cacheSaturated[str(speed)+"gbps"] = {}
	for hitrate in [round(x * 0.1, 1) for x in range(0, 11)]:
		cacheSaturated[str(speed)+"gbps"]["hitrate"+str(hitrate)] = {}
		hitrateDir = os.path.join(args.datadir, f"individualJobs/*_Gateway{speed}Gbps_HDDcache*/clusterid_*_hitrate_{hitrate}.csv")
		#print(hitrateDir,glob.glob(hitrateDir))
		for datafile in glob.glob(hitrateDir):
			#print(datafile)
			if "Bug_" in datafile or "corrupted" in datafile:
				continue
			with open(datafile) as f:
				reader = csv.DictReader(f, delimiter=',')
				reader.fieldnames = [field.strip() for field in reader.fieldnames]
				for row in reader:
					#print(row)
					machine = row['machine.name'].strip()
					runtime = float(row['job.runtime'])
					if machine not in cacheSaturated[str(speed)+"gbps"]["hitrate"+str(hitrate)]:
						cacheSaturated[str(speed)+"gbps"]["hitrate"+str(hitrate)][machine] = {'runtimes': [], 'average': None, 'stdev': None}
					cacheSaturated[str(speed)+"gbps"]["hitrate"+str(hitrate)][machine]['runtimes'].append(runtime)
		for machine in cacheSaturated[str(speed)+"gbps"]["hitrate"+str(hitrate)]:
			runtimes = cacheSaturated[str(speed)+"gbps"]["hitrate"+str(hitrate)][machine]['runtimes']
			if runtimes:
				average = sum(runtimes) / len(runtimes)
				stdev = statistics.stdev(runtimes) if len(runtimes) > 1 else None
				cacheSaturated[str(speed)+"gbps"]["hitrate"+str(hitrate)][machine]['average'] = average
				cacheSaturated[str(speed)+"gbps"]["hitrate"+str(hitrate)][machine]['stdev'] = stdev
				cacheSaturated[str(speed)+"gbps"]["hitrate"+str(hitrate)][machine].pop('runtimes')

if args.csv:
	if args.output is None:
		args.output = 'output.csv'
	with open(args.output, 'w') as csvfile:
		writer = csv.writer(csvfile)
		writer.writerow(['Speed', 'Hitrate', 'Machine', 'Average runtime', 'Standard deviation'])
		for speed in cacheSaturated:
			for hitrate in cacheSaturated[speed]:
				for machine in cacheSaturated[speed][hitrate]:
					avg = cacheSaturated[speed][hitrate][machine]['average']
					stdev = cacheSaturated[speed][hitrate][machine]['stdev']
					writer.writerow([speed, hitrate, machine, avg, stdev])
else:
	if args.output is None:
		args.output = 'output.json'
	with open(args.output, 'w') as f:
		json.dump(cacheSaturated, f, indent=4)


