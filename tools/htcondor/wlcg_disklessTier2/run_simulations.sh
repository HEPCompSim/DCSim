#! /bin/bash

ulimit -s unlimited
set -e

echo "INITIAL ENVIRONMENT START"
env
echo "INITIAL ENVIRONMENT END"
echo ""

echo "CURRENT DIRECTORY CONTENT:"
ls $(pwd)

#echo "SETTING GRID ENVIRONMENT"
#source /cvmfs/grid.cern.ch/umd-c7ui-latest/etc/profile.d/setup-c7-ui-example.sh

#echo "GETTING CONDA ENVIRONMENT FROM REMOTE STORAGE"
#xrdcp root://cmsxrootd-kit-disk.gridka.de:1094/pnfs/gridka.de/cms/disk-only/store/user/mhorzela/dcsim-env.tar.gz dcsim-env.tar.gz
## gfal-copy davs://cmswebdav-kit.gridka.de:2880/pnfs/gridka.de/cms/disk-only/store/user/aakhmets/dcsim-env.tar.gz dcsim-env.tar.gz
#echo "EXTRACTING AND SETTING CONDA ENVIRONMENT"
#mkdir -p dcsim-env
#tar -zxvf dcsim-env.tar.gz -C dcsim-env
#
#source dcsim-env/bin/activate
#conda-unpack
#
#echo "UPDATING LIBRARIES"
#
#echo "Old LD_LIBRARY_PATH: ${LD_LIBRARY_PATH}"
#export LD_LIBRARY_PATH=${CONDA_PREFIX}/lib:${CONDA_PREFIX}/lib64:${CONDA_PREFIX}/lib32:${LD_LIBRARY_PATH}
#
#ldd ${CONDA_PREFIX}/bin/dc-sim
PLATFORM="${1}"
WORKLOADS="${2}"
DATASETS="${3}"
SEED="${5}"
BUFFERSIZE=0
DUPLICATIONS=1
HITRATE="${4}"
XRDBLOCKSIZE=100000000

echo "Starting execution of simulation..."

#    sgbatch-sim --platform ${PLATFORM}.xml \
    dc-sim --platform ${PLATFORM} \
        --workload-configurations ${WORKLOADS} \
        --dataset-configurations ${DATASETS} \
        --duplications ${DUPLICATIONS} \
	--cfg=network/loopback-bw:100000000000000 \
        --cache-scope network \
	--no-caching \
        --hitrate ${HITRATE}\
        --xrd-blocksize ${XRDBLOCKSIZE} \
        --storage-buffer-size ${BUFFERSIZE} \
        --output-file "${PLATFORM}_H${HITRATE}_S${SEED}.csv" 
#    & TEST_PID=$!

#    (while [[ True ]]; \
#        do ps -aux | grep " ${TEST_PID} " | grep "dc-sim" \
#        >> scaling_dump_${PLATFORM}${NJOBS}jobs.txt; \
#        truncate -s-1 scaling_dump_${PLATFORM}${NJOBS}jobs.txt; \
#       	echo "${NJOBS}" >> scaling_dump_${PLATFORM}${NJOBS}jobs.txt; \
#        sleep 10; done;)\
#    & MONITOR_PID=$!
#    echo "Simulation process to monitor: $TEST_PID"
#    echo "Monitoring process: $MONITOR_PID"
#
#    wait $TEST_PID
#    kill -9 ${MONITOR_PID}
