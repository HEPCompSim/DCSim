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

    local PLATFORM_DIR="$parent/data/platform-files"
    local PLATFORM="sgbatch.xml"

    local NJOBS=60
    local NINFILES=10
    local AVGINSIZE=3600000000
    local FLOPS=216442800000
    local SIGMA_FLOPS=0
    local SIGMA_MEM=0
    local SIGMA_INSIZE=0
    local SIGMA_OUTSIZE=0

    local SCENARIO="fullstream" # further options synchronized with plotting script "copy", "simplifiedstream", "fullstream"

    local OUTDIR="$parent/tmp/outputs"
    if [ ! -d $OUTDIR ]; then
        mkdir -p $OUTDIR
    fi

    for hitrate in $(LANG=en_US seq 0.0 0.1 1.0)
    do 
        sgbatch-sim --platform "$PLATFORM_DIR/$PLATFORM" \
            --njobs $NJOBS \
            --ninfiles $NINFILES --insize $AVGINSIZE \
            --sigma-insize $SIGMA_INSIZE \
            --hitrate ${hitrate} \
            --flops $FLOPS \
            --sigma-flops $SIGMA_FLOPS \
            --sigma-mem $SIGMA_MEM \
            --sigma-outsize $SIGMA_OUTSIZE \
            --output-file ${OUTDIR}/hitratescaling_${SCENARIO}_${NJOBS}jobs_hitrate${hitrate}.csv
    done
}

action "$@"
