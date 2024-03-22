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
#BUFFER_SIZE=0
BUFFER_SIZE=0

if [ ! -d "tmp/monitor/$SCENARIO" ]; then
    mkdir -p tmp/monitor/$SCENARIO
fi


for NJOBS in 1
do
    for XRDBLOCKSIZE in 1000000 10000000 100000000 500000000 1000000000 5000000000
    #for XRDBLOCKSIZE in 10000000 5000000000
    #for XRDBLOCKSIZE in 1000000000000000
    do
	echo "XRDBLOCKSIZE: $XRDBLOCKSIZE"
    	WORKLOAD_TMP="${WORKLOAD}.tmp" 
    	cp $WORKLOAD $WORKLOAD_TMP
    	DATASET_TMP="${DATASET}.tmp"
    	cp $DATASET $DATASET_TMP
    	NFILES=$((NJOBS*1))
    	sed -i "" "s/\"num_jobs\": [0-9]*,/\"num_jobs\": $NJOBS,/" "$WORKLOAD_TMP"
    	sed -i "" "s/\"num_files\": [0-9]*,/\"num_files\": $NFILES,/" "$DATASET_TMP"
	OUTFILE=/tmp/benchmark_xrtd_$XRDBLOCKSIZE.stdout
	ERRFILE=/tmp/benchmark_xrtd_$XRDBLOCKSIZE.stderr
    	/opt/local/bin/gtime -v dc-sim --platform "$PLATFORM" \
        	--duplications ${DUPLICATIONS} \
        	--hitrate 0.0 \
		--storage-buffer-size $BUFFER_SIZE \
        	--xrd-blocksize ${XRDBLOCKSIZE} \
        	--output-file /dev/null \
        	--workload-configurations "$WORKLOAD_TMP" \
        	--dataset-configurations "$DATASET_TMP"  --wrench-full-log 1> $OUTFILE 2> $ERRFILE
    	cat $ERRFILE | grep -e "done" | sed "s/S.* /$NJOBS jobs took /" | sed "s/$/ sec/"
    	cat $ERRFILE | grep -e "Elap" | sed "s/.*):/  - Elapsed:    /"
    	cat $ERRFILE | grep -e "Maxi" | sed "s/.*):/  - MaxRSS (kb):/"
	echo "Detailed RSS info: cat $ERRFILE | grep CURRENT"
   
	done
done
