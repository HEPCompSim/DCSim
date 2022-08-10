#! /bin/bash

# script for execution of simulation scenarios to test the runtime and memory scaling of the simulator
#
#

NJOBS=60
NINFILES=10
AVGINSIZE=3600000000

SCENARIO="ETPbatch"

if [ ! -d "tmp/monitor/$SCENARIO" ]; then
    mkdir -p tmp/monitor/$SCENARIO
fi

for NJOBS in 10 20 50 100 200 500 700 1000 1100 1200 1300 1500 1700 2000 2500 3000 
do
    sgbatch-sim --platform data/platform-files/ETPbatch.xml \
        --njobs ${NJOBS} --ninfiles ${NINFILES} --insize ${AVGINSIZE} \
        --hitrate 0.0 \
        --output-file /dev/null \
    & TEST_PID=$!

    (while [[ True ]]; \
        do ps -aux | grep " ${TEST_PID} " | grep "sgbatch-sim" \
        >> tmp/monitor/$SCENARIO/test_privatedump_${NJOBS}jobs.txt; \
        sleep 10; done;)\
    & MONITOR_PID=$!
    echo "Simulation process to monitor: $TEST_PID"
    echo "Monitoring process: $MONITOR_PID"

    wait $TEST_PID
    kill -9 ${MONITOR_PID}

    sysctl vm.drop_caches=3
done
