#! /bin/bash

ulimit -s unlimited
set -e

echo "INITIAL ENVIRONMENT START"
env
echo "INITIAL ENVIRONMENT END"
echo ""

echo "CURRENT DIRECTORY CONTENT:"
ls $(pwd)

echo "SETTING GRID ENVIRONMENT"
source /cvmfs/grid.cern.ch/umd-c7ui-latest/etc/profile.d/setup-c7-ui-example.sh

# echo "SOURCING CONDA ENVIRONMENT FROM CVMFS"
# source /cvmfs/etp.kit.edu/DCSim/pre_0.2/setup.sh

echo "GETTING CONDA ENVIRONMENT FROM REMOTE STORAGE"
gfal-copy davs://cmswebdav-kit.gridka.de:2880/pnfs/gridka.de/cms/disk-only/store/user/mhorzela/dcsim-env.tar.gz dcsim-env.tar.gz

echo "EXTRACTING AND SETTING CONDA ENVIRONMENT"
mkdir -p dcsim-env
tar -zxvf dcsim-env.tar.gz -C dcsim-env

source dcsim-env/bin/activate
conda-unpack

echo "UPDATING LIBRARIES"

echo "Old LD_LIBRARY_PATH: ${LD_LIBRARY_PATH}"
export LD_LIBRARY_PATH=${CONDA_PREFIX}/lib:${CONDA_PREFIX}/lib64:${CONDA_PREFIX}/lib32:${LD_LIBRARY_PATH}

ldd ${CONDA_PREFIX}/bin/dc-sim

echo "START SIMULATION"
DUPLICATIONS=1
PREFETCHRATE="${1}"
XRDBLOCKSIZE=100000000
STORAGE_BUFFER_SIZE=0 #1048576

PLATFORM="WLCG_disklessTier2_reduced50.xml"
WORKLOADS="T?_DE_*_workloads.json"

SCENARIO="prefetchScanScaled50"

    dc-sim --platform "$PLATFORM" \
            --hitrate $PREFETCHRATE \
            --xrd-blocksize $XRDBLOCKSIZE \
            --output-file ${SCENARIO}_rate$PREFETCHRATE.csv \
            --cfg=network/loopback-bw:100000000000000 \
            --storage-buffer-size $STORAGE_BUFFER_SIZE \
            --cache-scope network \
            --no-caching \
            --duplications $DUPLICATIONS \
            --workload-configurations $WORKLOADS \
    & TEST_PID=$!

    (while [[ True ]]; \
        do ps -aux | grep " ${TEST_PID} " | grep "dc-sim" \
        >> scaling_dump_${PLATFORM}_rate${PREFETCHRATE}.txt; \
        sleep 10; done;)\
    & MONITOR_PID=$!
    echo "Simulation process to monitor: $TEST_PID"
    echo "Monitoring process: $MONITOR_PID"

    wait $TEST_PID
    kill -9 ${MONITOR_PID}
