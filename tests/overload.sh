#!/bin/sh

MAX=1024
SLEEP=15

if [ $# -ne 3 ]; then
    echo "Usage: $0 <output_dir> <parallel_tasks> <sched_policy>"
    exit 1
fi

output_dir=$1
tasks=$2
sched_policy=$3

# Start clean
make clean > /dev/null 2>&1 && make > /dev/null 2>&1

# Start the orchestrator
bin/orchestrator "$output_dir" "$tasks" "$sched_policy" &

# Start the overload, send max + tasks + 1 requests
for _ in $(seq 1 $((MAX + tasks + 1))); do
    bin/client execute 10000 -u "sleep 10"
done

# print the number of requests
echo "overload.sh: Number of requests sent: $((MAX + tasks + 1))"
echo "overload.sh: Sending kill signal in $SLEEP seconds"

# sleep for a while
sleep $SLEEP

# Kill the server
bin/client kill

# Some errors will pop up, because of the finished jobs
# are sending completion messages to the orchestrator
exit 0
