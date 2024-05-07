#!/bin/bash
# This script is used to demonstrate the execution
# of valid commands, it uses the diff command to
# compare the output and errors of the command
# executed in the server and the respective native
# command.
#
# You must run the server before running this script:
# bin/orchestrator <output_dir> <parallel_tasks> [sched_policy]

# Colors
G='\033[1;32m'  # Green
C='\033[0;36m'  # Cyan
R='\033[0;31m'  # Red
N='\033[0m'  # No Color

## Receive the output directory as argument
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <output_dir>"
    exit 1
fi

# Execute invalid commands from file: cmds_valid.txt
while read -r set; do
    sleep_time=$(echo "$set" | cut -d' ' -f1)
    pipe_flag=$(echo "$set" | cut -d' ' -f2)
    cmd=$(echo "$set" | cut -d' ' -f3-)

    echo -e "${C}Executing command:${N} \"$cmd\""
    # Execute the command and save the output and errors
    sh -c "$cmd" > /tmp/expected_output.txt 2> /tmp/expected_errors.txt

    # Execute the command in the server
    bin/client execute "$sleep_time" "$pipe_flag" "$cmd"
    sleep "$sleep_time"

    echo ""

    # Find the last task output directory created
    output_dir=$(find "$1" -type d -name "task*" | sort -V | tail -n 1)

    # Compare the output with diff
    if diff /tmp/expected_output.txt "$output_dir/out"; then
        echo -e "${G}Output is correct!${N}"
    else
        echo -e "${R}Output is incorrect!${N}"
    fi

    # Compare the errors with diff
    if diff /tmp/expected_errors.txt "$output_dir/err"; then
        echo -e "${G}Errors are correct!${N}"
    else
        echo -e "${R}Errors are incorrect!${N}"
    fi

    echo ""
    echo "-----------------------------------------------"
    echo ""
done < tests/cmds_valid.txt

exit 0
