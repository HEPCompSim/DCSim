NJOBS="${1}"
NINFILES=20
AVGINSIZE=$(bc -l <<< "8554379000/20")
SIGMAINSIZE=$(bc -l <<< "10000")
FLOPS=$(bc -l <<< "1.95*1480*1000*1000*1000")
SIGMAFLOPS=10000000
MEM=2400
OUTSIZE=$(bc -l <<< "50000000")
SIGMAOUTSIZE=$(bc -l <<< "1000000")
DUPLICATIONS=1
HITRATE=0.05
XRDBLOCKSIZE=1000000

PLATFORM="../data/platform-files/ETPbatch"

    ./dc-sim --platform ${PLATFORM}.xml \
        --njobs ${NJOBS} --ninfiles ${NINFILES} --insize ${AVGINSIZE} --sigma-insize ${SIGMAINSIZE} \
        --flops ${FLOPS} --sigma-flops ${SIGMAFLOPS} --mem ${MEM} \
        --outsize ${OUTSIZE} --sigma-outsize ${SIGMAOUTSIZE} \
        --duplications ${DUPLICATIONS} \
        --hitrate 0.0 \
        --xrd-blocksize ${XRDBLOCKSIZE} \
        --output-file ${PLATFORM}${NJOBS}.csv --wrench-full-log
