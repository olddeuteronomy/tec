#!/bin/bash

prog=out/tcp_client_clang
# prog=out/tcp_client_nd_clang
nruns=5000
sec_to_sleep=0.05

for ((i = 1; i <= $nruns; i++)); do
    echo "Run $i of $nruns: executing $prog"

    $prog

    exit_status=$?

    if [ $exit_status -ne 0 ]; then
        echo "$prog failed with exit status $exit_status on run $i. Stopping."
        exit $exit_status
    fi

    # Sleep only if this wasn't the last iteration
    if [ $i -lt $nruns ]; then
        sleep $sec_to_sleep
    fi
done

echo "All $nruns runs completed successfully."
