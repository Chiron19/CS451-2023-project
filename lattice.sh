#!/bin/bash

parameters() {
    RUNSCRIPT="./template_cpp/run.sh"
    LOGSDIR=./example/output
}

build() {
    ./template_cpp/cleanup.sh
    ./template_cpp/build.sh
}

body() {
    PROCESSES=3
    PROPOSALS=10
    PROPOSAL_MAX_VALUES=3
    PROPOSALS_DISTINCT_VALUES=5
    ./tools/stress.py agreement -r $RUNSCRIPT -l $LOGSDIR -p $PROCESSES -n $PROPOSALS -v $PROPOSAL_MAX_VALUES -d $PROPOSALS_DISTINCT_VALUES
}

validate() {
    $LOGSDIR/verify_lattice $LOGSDIR
}

run_build=false
run_body=true
run_validate=false

while getopts ":bBvV" option; do
    case $option in
        b) # build
            run_build=true
            ;;
        B) # build only
            run_build=true
            run_body=false
            ;;
        v) # validate
            run_validate=true
            ;;
        V) # validate only
            run_validate=true
            run_body=false
            ;;
        \?) # Invalid option
            echo "Error: Invalid option"
            exit;;
    esac
done

parameters

if [ "$run_build" = true ]; then
    build
fi

if [ "$run_body" = true ]; then
    body
fi

if [ "$run_validate" = true ]; then
    validate
fi