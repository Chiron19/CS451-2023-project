RUNSCRIPT="./template_cpp/run.sh"
LOGSDIR="./example/output"
PROCESSES=3
MESSAGES=10
./tools/stress.py fifo -r $RUNSCRIPT -l $LOGSDIR -p $PROCESSES -m $MESSAGES