#! /bin/bash

# script for profing the memory heap consumption with valgrind of our caching example (using 100 jobs)
#
#

valgrind --tool=massif --time-unit=ms --massif-out-file="massif.out" my-executable DistCacheSim/sgbatch/data/platform-files/hosts.xml 100 10 3600000000 0

# the output massif.out can then be visualized for example with 'massif-visualizer':
#
# https://apps.kde.org/massif-visualizer/
# 
# which can be installed on ubuntu via snap (or Ubuntu Software Store)
