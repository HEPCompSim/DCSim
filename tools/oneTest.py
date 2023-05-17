#! /usr/bin/python3
from platformFromVector import pFromV
from sumarize_sim import extract
import sys

import subprocess
def oneTest(xml_file_path, cpu_speed, read_speed, link_speed, net_speed,hitrates):
	hits=' '.join([str(i) for i in hitrates])
	platform=pFromV(xml_file_path, cpu_speed, read_speed, link_speed, net_speed)
	process = subprocess.run(["./hitrateScanScript.sh",platform,hits])
	return extract("../tmp/outputs")
def main():
	xml_file_path, cpu_speed, read_speed, link_speed, net_speed = sys.argv[1:]
	print(oneTest(xml_file_path, cpu_speed, read_speed, link_speed, net_speed,list(range(10))))
if __name__ == '__main__':
	main()
