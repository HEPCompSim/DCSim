#! /bin/bash

bash tools/hitratescan_debug.sh
pushd tmp/outputs/
python3 ~/DistCacheSim/tools/hitrateperformance.py --scenario fullstream --suffix "_validationSGbatch_CPUbound" hitratescaling_fullstream_*jobs_hitrate*.csv 
popd