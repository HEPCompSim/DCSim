#! /bin/bash

# script for execution of simulation scenarios to test the runtime and memory scaling of the simulator
#
#

SCENARIO="private"

if [ ! -d "tmp/monitor/$SCENARIO" ]; then
    mkdir -p tmp/monitor/$SCENARIO
fi

for NJOBS in 1500 2000 2500 3000 
do
    ./my-executable data/platform-files/host_scaletest.xml ${NJOBS} 10 3600000000 0 & TEST_PID=$!
    echo $TEST_PID
    (while [[ True ]]; do ps -aux | grep " ${TEST_PID} " | grep "my" >> tmp/monitor/$SCENARIO/test_privatedump_${NJOBS}jobs.txt; sleep 10; done;)& MONITOR_PID=$!
    echo $TEST_PID $MONITOR_PID
    wait $TEST_PID
    kill -9 ${MONITOR_PID}
done