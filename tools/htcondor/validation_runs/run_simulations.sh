#! /bin/bash

ulimit -s unlimited
set -e

echo "INITIAL ENVIRONMENT START"
env
echo "INITIAL ENVIRONMENT END"
echo ""

echo "CURRENT DIRECTORY CONTENT:"
ls $(pwd)

python3 run_shell_simulations.py \
    --platform "$1" \
    --workload "$2" \
    --dataset "$3" \
    --xrootd_blocksize "$4" \
    --storage_buffer_size "$5" \
    --seed "$6" \
    --shell "$7" \
    --from-line "$8" \
    --to-line "$9"
