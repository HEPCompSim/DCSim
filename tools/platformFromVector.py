#!/usr/bin/env python3
import sys
import re
import shlex
def pFromV(xml_file_path, cpu_speed, read_speed, link_speed, net_speed):
	# Get the command-line arguments
	

	# Read the contents of the XML file
	with open(xml_file_path, 'r') as f:
		xml_contents = f.read()

	# Replace the placeholders in the XML file with the specified values
	xml_contents = re.sub(r'{cpu-speed}', str(cpu_speed), xml_contents)
	xml_contents = re.sub(r'{read-speed}', str(read_speed), xml_contents)
	xml_contents = re.sub(r'{link-speed}', str(link_speed), xml_contents)
	xml_contents = re.sub(r'{net-speed}', str(net_speed), xml_contents)

	return xml_contents

if __name__ == '__main__':
	xml_file_path, cpu_speed, read_speed, link_speed, net_speed = sys.argv[1:]
	print(pFromV(xml_file_path, cpu_speed, read_speed, link_speed, net_speed)) 
