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

    local PLATFORM="$parent/data/platform-files/sgbatch_validation.xml"
    local WORKLOAD="$parent/data/workflow-configs/crown_ttbar_validation.json"

    local NJOBS=1
    local NINFILES=20 #10
    local AVGINSIZE=$(bc -l <<< "8554379000 / ${NINFILES}")
    local AVGOUTSIZE=16000000
    local FLOPS=$(bc -l <<< "1.95*1480*1000*1000*1000")
    local MEM=2400
    local SIGMA_FLOPS=0
    local SIGMA_MEM=0
    local SIGMA_INSIZE=0
    local SIGMA_OUTSIZE=0

    local DUPLICATIONS=48

    local XRD_BLOCKSIZE=100000000

    local SCENARIO="fullstream" # further options synchronized with plotting script "copy", "simplifiedstream", "fullstream"

    local OUTDIR="$parent/tmp/outputs"
    if [ ! -d $OUTDIR ]; then
        mkdir -p $OUTDIR
    fi

    for hitrate in $(LANG=en_US seq 0.0 0.1 1.0)
    do 
        dc-sim --platform "$PLATFORM" \
            --hitrate ${hitrate} \
            --duplications $DUPLICATIONS \
            --xrd-blocksize $XRD_BLOCKSIZE \
            --output-file ${OUTDIR}/hitratescaling_${SCENARIO}_xrd${XRD_BLOCKSIZE}_${NJOBS}jobs_hitrate${hitrate}.csv \
            --cfg=network/loopback-bw:100000000000000 \
            --no-caching \
            --workflow-configurations "$WORKLOAD" #\
            # --njobs $NJOBS \
            # --ninfiles $NINFILES \
            # --insize $AVGINSIZE \
            # --sigma-insize $SIGMA_INSIZE \
            # --flops $FLOPS \
            # --sigma-flops $SIGMA_FLOPS \
            # --mem $MEM \
            # --sigma-mem $SIGMA_MEM \
            # --outsize $AVGOUTSIZE \
            # --sigma-outsize $SIGMA_OUTSIZE \
            # --no-streaming \
            # --wrench-full-log
            # --log=simple_wms.threshold=debug \
            # --log=cache_computation.threshold=debug
    done
}

action "$@"
