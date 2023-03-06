#! /bin/bash

# Bash script tu run simulations for several hitrate values.
# The simulation monitoring outputs can be plotted using the hitrateperfromance.py plotting script.

action() {
    # determine the directy of this file
    if [ ! -z "$ZSH_VERSION" ]; then
        local this_file="${(%):-%x}"
    else
        local this_file="${BASH_SOURCE[0]}"
    fi
    #source /cvmfs/cms.cern.ch/slc6_amd64_gcc480/external/python/2.7.3/etc/profile.d/init.sh

    local base="$( cd "$( dirname "$this_file" )" && pwd )"
    local parent="$( dirname "$base" )"

    local PLATFORM="$parent/data/platform-files/WLCG_disklessTier2_reduced100.xml"
    # local PLATFORM="$parent/data/platform-files/WLCG_disklessTier2_reduced1000.xml"
    # local WORKLOADS="$parent/data/workload-configs/Dummy_workloads.json $parent/data/workload-configs/T?_DE_*_workloads.json"
    local WORKLOADS="$parent/data/workload-configs/T?_DE_*_workloads.json"

    local XRD_BLOCKSIZE=100000000
    local STORAGE_BUFFER_SIZE=0 #1048576

    local SCENARIO="prefetchScanScaled100"
    # local SCENARIO="prefetchScanScaled1000"

    local OUTDIR="$parent/tmp/outputs/WLCG"
    if [ ! -d $OUTDIR ]; then
        mkdir -p $OUTDIR
    fi

    for prefetchrate in $(LANG=en_US seq 0.0 0.1 1.0)
    do 
        dc-sim --platform "$PLATFORM" \
            --hitrate ${prefetchrate} \
            --xrd-blocksize $XRD_BLOCKSIZE \
            --output-file ${OUTDIR}/${SCENARIO}_rate${prefetchrate}.csv \
            --cfg=network/loopback-bw:100000000000000 \
            --storage-buffer-size $STORAGE_BUFFER_SIZE \
            --cache-scope network \
            --no-caching \
            --workload-configurations $WORKLOADS #\
            # --no-streaming \
            # --wrench-full-log
            # --log=simple_wms.threshold=debug \
            # --log=cache_computation.threshold=debug
    done
}

action "$@"
