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
    # --platform "$1" \
    # --workload "$2" \
    # --dataset "$3" \
    --shell "$1" \
    --from-line "$2" \
    --to-line "$3"
