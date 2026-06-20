#!/bin/bash

# Parameters
START_INDEX=${2:-0}
END_INDEX=${1:-20}
STEP=10

TEMPLATE=run_simulations.sub.template
SUBMIT_FILE=jobs.sub
JOBS_FILE=$(mktemp)

# Generate job lines
for (( i=$START_INDEX; i<$END_INDEX; i+=$STEP )); do
    START=$i
    END=$((i + STEP))
    if [ "$END" -gt "$END_INDEX" ]; then END=$END_INDEX; fi
    echo "    $START, $END" >> "$JOBS_FILE"
done

# Replace placeholder in template with contents of jobs file
sed -e "/<JOBS>/r $JOBS_FILE" -e "/<JOBS>/d" "$TEMPLATE" > "$SUBMIT_FILE"

rm "$JOBS_FILE"

echo "Submit file generated: $SUBMIT_FILE"
