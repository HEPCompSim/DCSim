#!/usr/bin/env python3
import os
import pandas as pd
import argparse

def update_csv_files(directory):
	for root, _, files in os.walk(directory):
		for file in files:
			if file.endswith('.csv'):
				file_path = os.path.join(root, file)
				try:
					df = pd.read_csv(file_path)
					if ' job.computetime' in df.columns:
						df[' job.computetime'] = 0
						df.to_csv(file_path, index=False)
						print(f"Updated {file_path}")
					else:
						print(f"'job.computetime' column not found in {file_path}")
				except Exception as e:
					print(f"Error processing {file_path}: {e}")

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='Recursively update CSV files in a directory.')
	parser.add_argument('directory', type=str, help='The directory to search for CSV files')
	args = parser.parse_args()

	update_csv_files(args.directory)
