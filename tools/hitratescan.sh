#! /bin/bash

# Bash script tu run simulations for several hitrate values.
# The simulation monitoring outputs can be plotted using the hitrateperfromance.py plotting script.

NJOBS=60
NINFILES=10
AVGINSIZE=3600000000
FLOPS=216442800000

SCENARIO="fullstream" # further options synchronized with plotting script "copy", "simplifiedstream", "fullstream"

OUTDIR="tmp/outputs"
if [ ! -d $OUTDIR ]; then
    mkdir -p $OUTDIR
fi

for hitrate in $(LANG=en_US seq 0.0 0.1 1.0)
do 
    sgbatch-sim --platform data/platform-files/hosts.xml \
        --njobs $NJOBS \
        --ninfiles $NINFILES --insize $AVGINSIZE \
        --hitrate ${hitrate} \
        --flops $FLOPS \
        --output-file ${OUTDIR}/hitratescaling_${SCENARIO}_${NJOBS}jobs_hitrate${hitrate}.csv
done