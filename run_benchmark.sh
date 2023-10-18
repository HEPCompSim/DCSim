if [ ! $# -eq 2 ]; then
    echo "Usage: ${0} <# of jobs> <platform file>"
    exit 1
fi


NJOBS="${1}"
PLATFORM="${2}"


./build/dc-sim --platform ${PLATFORM} \
        --njobs ${NJOBS} --ninfiles 20 --insize 427718950 --sigma-insize 10000 \
        --flops 2886000000000 --sigma-flops 10000000 \
        --outsize 50000000 --sigma-outsize 1000000 \
        --xrd-blocksize 1000000 \
        --storage-buffer-size 0 \
        --output-file /dev/null \
	--wrench-default-control-message-size=0

