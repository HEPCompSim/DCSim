if [ ! $# -eq 4 ]; then
    echo "Usage: ${0} <# of jobs> <# of files per job> <executable> <buffer size>"
    exit 1
fi


NJOBS="${1}"
NINFILES="${2}"
EXEC="${3}"
BUFFERSIZE="${4}"
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


PLATFORM="../data/platform-files/ETPbatch_faster"

./${EXEC} --platform ${PLATFORM}.xml \
        --njobs ${NJOBS} --ninfiles ${NINFILES} --insize ${AVGINSIZE} --sigma-insize ${SIGMAINSIZE} \
        --flops ${FLOPS} --sigma-flops ${SIGMAFLOPS} --mem ${MEM} \
        --outsize ${OUTSIZE} --sigma-outsize ${SIGMAOUTSIZE} \
        --duplications ${DUPLICATIONS} \
        --hitrate 0.0 \
 	--storage-buffer-size ${BUFFERSIZE} \
        --xrd-blocksize ${XRDBLOCKSIZE} \
        --output-file ${PLATFORM}${NJOBS}.csv  
 
