#! /bin/bash

bash tools/hitratescan.sh
pushd tmp/outputs/
python3 ~/DistCacheSim/tools/hitrateperformance.py --scenario fullstream --suffix "_validationSGbatch_CPUbound" hitratescaling_fullstream_48jobs_hitrate*.csv 
popd