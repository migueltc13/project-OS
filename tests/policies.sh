#!/bin/bash

if [ $# -ne 3 ]; then
    echo "Usage: $0 <output_dir> <parallel_tasks> <sched_policy>"
    exit 1
fi

export output_dir=$1
export tasks=$2
export sched_policy=$3

# Start clean
make clean &> /dev/null && make &> /dev/null

# Start the orchestrator
bin/orchestrator "$output_dir" "$tasks" "$sched_policy" &

# Fill the execution queue
for _ in $(seq 1 "$tasks"); do
    cmd="echo fill | sleep 2"
    bin/client execute 2000 -p "$cmd"
done

# Execute the tasks to check policy order
sleep_time=1
for _ in $(seq 1 "$tasks"); do
    # random sleep time between 1 and 4 seconds
    random=$((1 + RANDOM % 4))
    cmd="sleep $random"
    echo "Testing: bin/client execute $((random * 1000)) -u \"$cmd\""
    bin/client execute $((random * 1000)) -u "$cmd"
    sleep_time=$((sleep_time + random))
done

# Wait for the tasks to finish
sleep "$sleep_time"

# Get the status of the tasks
bin/client status

# Sleep for 1 second to allow the orchestrator to show the status
sleep 1

# Kill the orchestrator
bin/client kill
exit 0
