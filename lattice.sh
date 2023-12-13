#!/bin/bash

build() {
    ./template_cpp/cleanup.sh
    ./template_cpp/build.sh
}

run_body() {
    RUNSCRIPT="./template_cpp/run.sh"
    LOGSDIR=./example/output
    PROCESSES=3
    PROPOSALS=10
    PROPOSAL_MAX_VALUES=3
    PROPOSALS_DISTINCT_VALUES=5
    ./tools/stress.py agreement -r $RUNSCRIPT -l $LOGSDIR -p $PROCESSES -n $PROPOSALS -v $PROPOSAL_MAX_VALUES -d $PROPOSALS_DISTINCT_VALUES
}

run_build=false
run_validate=false

while getopts ":b" option; do
    case $option in
        b) # build
            run_build=true
            ;;
        \?) # Invalid option
            echo "Error: Invalid option"
            exit;;
    esac
done

if [ "$run_build" = true ]; then
    build
fi

run_body