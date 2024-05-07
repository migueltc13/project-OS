#!/bin/bash
# This script is used to demonstrate the execution
# of invalid commands, it prints the output and errors
# of the commands nad the respective server output and
# errors expected.
#
# You must run the server before running this script:
# bin/orchestrator <output_dir> <parallel_tasks> [sched_policy]
#
# Usage: ./cmds_invalid.sh <output_dir>

# Colors
G='\033[1;32m'  # Green
C='\033[0;36m'  # Cyan
N='\033[0m'  # No Color

## Receive the output directory as argument
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <output_dir>"
    exit 1
fi

# Execute invalid commands from file: cmds_invalid.txt
while read -r set; do
    sleep_time=$(echo "$set" | cut -d' ' -f1)
    pipe_flag=$(echo "$set" | cut -d' ' -f2)
    cmd=$(echo "$set" | cut -d' ' -f3-)

    echo -e "${G}Executing command:${N} \"$cmd\""
    # Execute the command and save the output and errors
    sh -c "$cmd" > /tmp/expected_output.txt 2> /tmp/expected_errors.txt

    # Execute the command in the server
    bin/client execute "$sleep_time" "$pipe_flag" "$cmd"
    sleep "$sleep_time"

    echo ""

    # Find the last task output directory created
    output_dir=$(find "$1" -type d -name "task*" | sort -V | tail -n 1)

    # Print the output of the command and the server
    echo -e "${C}Comparing output${N}"
    echo -e "${G}Server output:${N}"
    cat "$output_dir/out"
    echo -e "${G}Native output (expected):${N}"
    cat /tmp/expected_output.txt

    echo ""

    # Print the errors of the command and the server
    echo -e "${C}Comparing errors${N}"
    echo -e "${G}Server errors:${N}"
    cat "$output_dir/err"
    echo -e "${G}Native errors (expected):${N}"
    cat /tmp/expected_errors.txt

    echo ""
    echo "-----------------------------------------------"
    echo ""
done < tests/cmds_invalid.txt
