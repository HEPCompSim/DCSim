#! /bin/bash

# script for execution of simulation scenarios to test the runtime and memory scaling of the simulator
#
#
parent="$( dirname "$base" )"
PLATFORM1="$parent/data/platform-files/WLCG_disklessTier2.xml"
PLATFORM2="$parent/data/platform-files/WLCG_disklessTier2_Tier1_MODIFIED.xml"
PLATFORM3="$parent/data/platform-files/WLCG_disklessTier2_Tier1_Tier2_MODIFIED.xml"
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


for NJOBS in 400
do
    for XRDBLOCKSIZE in 1000000000
    do
	for PLATFORM in $PLATFORM1
	do
		echo "PLATFORM: $PLATFORM"
		PLATFORM_NAME=`echo $PLATFORM | sed "s/.*\///"`
    		WORKLOAD_TMP="${WORKLOAD}.tmp" 
    		cp $WORKLOAD $WORKLOAD_TMP
    		DATASET_TMP="${DATASET}.tmp"
    		cp $DATASET $DATASET_TMP
    		NFILES=$((NJOBS*20))
    		sed -i "" "s/\"num_jobs\": [0-9]*,/\"num_jobs\": $NJOBS,/" "$WORKLOAD_TMP"
    		sed -i "" "s/\"num_files\": [0-9]*,/\"num_files\": $NFILES,/" "$DATASET_TMP"
		OUTFILE=/tmp/benchmark_xrtd_$PLATFORM_NAME.stdout
		ERRFILE=/tmp/benchmark_xrtd_$PLATFORM_NAME.stderr
    		/opt/local/bin/gtime -v dc-sim --platform "$PLATFORM" \
        		--duplications ${DUPLICATIONS} \
        		--hitrate 1.0 \
			--storage-buffer-size $BUFFER_SIZE \
			--wrench-commport-pool-size=100000 \
        		--xrd-blocksize ${XRDBLOCKSIZE} \
        		--output-file /dev/null \
        		--wrench-full-log \
        		--workload-configurations "$WORKLOAD_TMP" \
        			--dataset-configurations "$DATASET_TMP"  1> $OUTFILE 2> $ERRFILE
    		cat $ERRFILE | grep -e "done" | sed "s/S.* /$NJOBS jobs took /" | sed "s/$/ sec/"
    		cat $ERRFILE | grep -e "Elap" | sed "s/.*):/  - Elapsed:    /"
    		cat $ERRFILE | grep -e "Maxi" | sed "s/.*):/  - MaxRSS (kb):/"
		echo "Detailed RSS info: cat $ERRFILE | grep CURRENT"
   
		done
	done
done
