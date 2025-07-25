#!/bin/bash

# Parameters
START_INDEX=0
END_INDEX=20
STEP=10

TEMPLATE=run_simulations.sub.template
SUBMIT_FILE=jobs.sub

# Start fresh
rm -f "$SUBMIT_FILE"

# Generate job entries for each chunk
for (( i=$START_INDEX; i<$END_INDEX; i+=$STEP )); do
    START=$i
    END=$((i + STEP))
    if [ "$END" -gt "$END_INDEX" ]; then END=$END_INDEX; fi

    # Replace placeholders and append to final submit file
    cat "$TEMPLATE" | sed "s/<FROM_LINE>/$START/g; s/<TO_LINE>/$END/g" >> "$SUBMIT_FILE"
    echo "" >> "$SUBMIT_FILE"  # Add newline between jobs
done

echo "Submit file generated: $SUBMIT_FILE"