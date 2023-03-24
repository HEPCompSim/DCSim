#! /usr/bin/python3
import os
import argparse
import csv
import json
from collections import defaultdict
from statistics import mean, stdev

def parse_csv(file):
	stats = defaultdict(list)
	with open(file, 'r') as f:
		reader = csv.DictReader(f)
		reader.fieldnames = [field.strip() for field in reader.fieldnames]
		for row in reader:
			machine = row['machine.name']
			runtime = float(row['job.end']) - float(row['job.start'])
			stats[machine].append(runtime)
	return stats

def calculate_stats(stats):
	return {
		'average': mean(stats),
		'stdev': stdev(stats)
	}

def write_output(output_file, data, csv_output=False):
	if csv_output:
		with open(output_file, 'w', newline='') as f:
			writer = csv.writer(f)
			writer.writerow(['hitrate', 'machine.name', 'average', 'stdev'])
			for hitrate, machines in data.items():
				for machine, stats in machines.items():
					writer.writerow([hitrate, machine.strip(), stats['average'], stats['stdev']])
	else:
		with open(output_file, 'w') as f:
			json.dump(data, f, indent=4, sort_keys=True)

def main():
	parser = argparse.ArgumentParser(description='Process CSV files in a directory to output hitrate/machine stats.')
	parser.add_argument('directory', type=str, help='the directory containing the CSV files')
	parser.add_argument('--csv','-c', action='store_true', help='output to CSV instead of JSON')
	parser.add_argument('--output','-o', type=str, help='output file name')
	args = parser.parse_args()

	hitrate_data = defaultdict(dict)
	for file in os.listdir(args.directory):
		if file.endswith('.csv'):
			hitrate = file.split('_')[4]
			stats = parse_csv(os.path.join(args.directory, file))
			for machine, machine_stats in stats.items():
				hitrate_data[hitrate][machine.strip()] = calculate_stats(machine_stats)

	if args.output is None:
		if args.csv:
			args.output = '../tmp/sim-summary.csv'
		else:
			args.output = '../tmp/sim-summary.json'

	write_output(args.output, hitrate_data, args.csv)
	print("output written to '"+os.path.abspath(args.output)+"'")
if __name__ == '__main__':
	main()

