#!/bin/sh
# Used to test the output contents of the orchestrator server
# writes both to stdout and stderr, also calculates the elapsed
# time in milliseconds

# Start time
start=$(date +%s.%N)

>&1 echo "This a success example"

sleep 1

>&2 echo "This is an error example"

# End time
end=$(date +%s.%N)

# Elapsed time in milliseconds
echo "($end - $start) * 1000" | bc
exit 0
