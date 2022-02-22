#! /bin/bash

# script for profing the memory heap consumption with valgrind of standard wrench example
# to make it work, please compile and install the examples in your wrench build via:
#
# make -j 6 install examples
# make install examples
#

# Change to your convenience:

WRENCH_SWDIR=./wrench
WRENCH_BUILDDIR=${WRENCH_SWDIR}/build

for njobs in 0 100 500 1000;
do
    valgrind --tool=massif --time-unit=ms --massif-out-file="massif_${njobs}_nolog_wrench2.0_test.out" \
    ${WRENCH_BUILDDIR}/examples/action_api/bare-metal-bag-of-actions/wrench-example-bare-metal-bag-of-actions ${njobs} ${WRENCH_SWDIR}/examples/action_api/bare-metal-bag-of-actions/two_hosts.xml;
done

# the outputs massif*.out can then be visualized for example with 'massif-visualizer':
#
# https://apps.kde.org/massif-visualizer/
# 
# which can be installed on ubuntu via snap (or Ubuntu Software Store)
