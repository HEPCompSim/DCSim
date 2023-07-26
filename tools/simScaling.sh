#! /bin/bash

# script for execution of simulation scenarios to test the runtime and memory scaling of the simulator
#
#
parent="$( dirname "$base" )"
PLATFORM="$parent/data/platform-files/WLCG_disklessTier2.xml"
WORKLOAD="$parent/data/workload-configs/simScaling.json"
DATASET="$parent/data/dataset-configs/simScaling.json"
DUPLICATIONS=1
HITRATE=0.0
XRDBLOCKSIZE=1000000000

SCENARIO="wlcg"

if [ ! -d "tmp/monitor/$SCENARIO" ]; then
    mkdir -p tmp/monitor/$SCENARIO
fi

for NJOBS in 10 20 50 100 200 500 700 1000 1100 1200 1300 1500 1700 2000
do

    dc-sim --platform "$PLATFORM" \
        --duplications ${DUPLICATIONS} \
        --hitrate 0.0 \
        --xrd-blocksize ${XRDBLOCKSIZE} \
        --output-file /dev/null \
        --workload-configurations "$WORKLOAD" \
        --dataset-configurations "$DATASET" \
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
