#!/usr/bin/env python3
import sys
import re
import shlex

# Get the command-line arguments
xml_file_path, cpu_speed, read_speed, link_speed, net_speed = sys.argv[1:]

# Read the contents of the XML file
with open(xml_file_path, 'r') as f:
    xml_contents = f.read()

# Replace the placeholders in the XML file with the specified values
xml_contents = re.sub(r'{CPU_SPEED}', cpu_speed, xml_contents)
xml_contents = re.sub(r'{READ_SPEED}', read_speed, xml_contents)
xml_contents = re.sub(r'{LINK_SPEED}', link_speed, xml_contents)
xml_contents = re.sub(r'{NET_SPEED}', net_speed, xml_contents)

print(xml_contents)
