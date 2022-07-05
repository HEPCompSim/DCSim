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
    local PLATFORM="sgbatch_weirdness.xml"

    local NJOBS=1
    local NINFILES=10 #10
    local AVGINSIZE=$(bc -l <<< "1000*1000*1000")
    local AVGOUTSIZE=0
    local FLOPS=1000
    local MEM=2400000000
    local SIGMA_FLOPS=0
    local SIGMA_MEM=0
    local SIGMA_INSIZE=0
    local SIGMA_OUTSIZE=0
    local DUPLICATIONS=1

    local XRD_BLOCKSIZE=1000000000

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
            --mem $MEM \
            --sigma-mem $SIGMA_MEM \
            --outsize $AVGOUTSIZE \
            --sigma-outsize $SIGMA_OUTSIZE \
            --duplications $DUPLICATIONS \
            --xrd-blocksize $XRD_BLOCKSIZE \
            --output-file ${OUTDIR}/hitratescaling_${SCENARIO}_debugjobs_hitrate${hitrate}.csv
    done
}

action "$@"
