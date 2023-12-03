#!/bin/bash

build() {
    ./template_cpp/cleanup.sh
    ./template_cpp/build.sh
}

validate() {
    ./tools/validate_fifo.py --proc_num 3 ./example/output/proc01.output ./example/output/proc02.output ./example/output/proc03.output
}

run_body() {
    RUNSCRIPT="./template_cpp/run.sh"
    LOGSDIR=./example/output
    PROCESSES=3
    MESSAGES=10
    ./tools/stress.py fifo -r $RUNSCRIPT -l $LOGSDIR -p $PROCESSES -m $MESSAGES
}

run_build=false
run_validate=false

while getopts ":bv" option; do
    case $option in
        b) # build
            run_build=true
            ;;
        v) # validate
            run_validate=true
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

if [ "$run_validate" = true ]; then
    validate
fi
