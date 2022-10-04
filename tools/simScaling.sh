#! /bin/bash

# script for execution of simulation scenarios to test the runtime and memory scaling of the simulator
#
#

NJOBS=60
NINFILES=10
AVGINSIZE=3600000000
SIGMAINSIZE=360000000
FLOPS=2164428000000
SIGMAFLOPS=216442800000
MEM=2000000000
OUTSIZE=18000000000
SIGMAOUTSIZE=1800000000
DUPLICATIONS=1
HITRATE=0.0
XRDBLOCKSIZE=1000000000

SCENARIO="ETPbatch"

if [ ! -d "tmp/monitor/$SCENARIO" ]; then
    mkdir -p tmp/monitor/$SCENARIO
fi

for NJOBS in 10 20 50 100 200 500 700 1000 1100 1200 1300 1500 1700 2000 2500 3000 
do
    dc-sim --platform data/platform-files/ETPbatch.xml \
        --njobs ${NJOBS} --ninfiles ${NINFILES} --insize ${AVGINSIZE} --sigma-insize ${SIGMAINSIZE} \
        --flops ${FLOPS} --sigma-flops ${SIGMAFLOPS} --mem ${MEM} \
        --outsize ${OUTSIZE} --sigma-outsize ${SIGMAOUTSIZE} \
        --duplications ${DUPLICATIONS} \
        --hitrate 0.0 \
        --xrd-blocksize ${XRDBLOCKSIZE} \
        --output-file /dev/null \
    & TEST_PID=$!

    (while [[ True ]]; \
        do ps -aux | grep " ${TEST_PID} " | grep "dc-sim" \
        >> tmp/monitor/$SCENARIO/test_privatedump_${NJOBS}jobs.txt; \
        sleep 10; done;)\
    & MONITOR_PID=$!
    echo "Simulation process to monitor: $TEST_PID"
    echo "Monitoring process: $MONITOR_PID"

    wait $TEST_PID
    kill -9 ${MONITOR_PID}

done
