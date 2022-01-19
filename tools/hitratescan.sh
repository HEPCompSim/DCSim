#! /bin/bash

NJOBS=60
NINFILES=10
AVGINSIZE=3600000000

SCENARIO="copy" # further options synchronized with plotting script "copy", "simplifiedstream", "fullstream"

OUTDIR="tmp/outputs"
if [ ! -d $OUTDIR ]; then
    mkdir -p $OUTDIR
fi

for hitrate in $(LANG=en_US seq 0.0 0.1 1.0)
do 
    sgbatch-sim --platform data/platform-files/hosts.xml --njobs $NJOBS --ninfiles $NINFILES --insize $AVGINSIZE --hitrate ${hitrate} --output-file ${OUTDIR}/hitratescaling_${SCENARIO}_${NJOBS}jobs_hitrate${hitrate}.csv --no-blockstreaming
done