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
#gfal-copy davs://cmswebdav-kit.gridka.de:2880/pnfs/gridka.de/cms/disk-only/store/user/mhorzela/dcsim-env.tar.gz dcsim-env.tar.gz
# gfal-copy davs://cmswebdav-kit.gridka.de:2880/pnfs/gridka.de/cms/disk-only/store/user/aakhmets/dcsim-env.tar.gz dcsim-env.tar.gz
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
WORKLOAD="${2}"
DATASET="${3}"

NJOBS="${4}"
NFILES=$((NJOBS*20))


BUFFERSIZE="${5}"
DUPLICATIONS=1
HITRATE=0.05
XRDBLOCKSIZE=1000000

sed -i "s/\"num_jobs\": [0-9]*,/\"num_jobs\": $NJOBS,/" "$WORKLOAD"
sed -i "s/\"num_files\": [0-9]*,/\"num_files\": $NFILES,/" "$DATASET"

#    sgbatch-sim --platform ${PLATFORM}.xml \
    dc-sim --platform ${PLATFORM} \
        --workload-configurations ${WORKLOAD} \
        --dataset-configurations ${DATASET} \
        --duplications ${DUPLICATIONS} \
        --hitrate 0.0 \
        --xrd-blocksize ${XRDBLOCKSIZE} \
        --storage-buffer-size ${BUFFERSIZE} \
        --output-file ${PLATFORM}${NJOBS}.csv \
    & TEST_PID=$!

    (while [[ True ]]; \
        do ps -aux | grep " ${TEST_PID} " | grep "dc-sim" \
        >> scaling_dump_${PLATFORM}${NJOBS}jobs.txt; \
        truncate -s-1 scaling_dump_${PLATFORM}${NJOBS}jobs.txt; \
       	echo "${NJOBS}" >> scaling_dump_${PLATFORM}${NJOBS}jobs.txt; \
        sleep 10; done;)\
    & MONITOR_PID=$!
    echo "Simulation process to monitor: $TEST_PID"
    echo "Monitoring process: $MONITOR_PID"

    wait $TEST_PID
    kill -9 ${MONITOR_PID}
