#!/bin/bash
#$ -S /bin/sh
#$ -e /home/yisu/Work/rflm-dev/err
#$ -o /home/yisu/Work/rflm-dev/out

source /home/yisu/Work/rflm-dev/scripts/vars.sh
source /home/yisu/Work/rflm-dev/scripts/functs.sh

export PATH=$sri/bin:$sri/bin/i686/:$PATH

####

smax=0
norand=0
simple=0
testrf=""
while getopts "ns:pt:" opt; do
    case $opt in
	s ) smax=$OPTARG;;
	n ) norand=1;;
	p ) simple=1;;
	t ) testrf=$OPTARG;;
	* ) echo "$0: bad option $@"; exit 1;;
    esac
done
shift $(($OPTIND-1))

if [ $# -lt 1 ]; then
    echo "Usage: $0 [opts] runName"
    echo " Opts: -s max     Shallow tree with max nodes"
    echo "       -n         No random selection"
    echo "       -p         Simple tree (backward compatible)"
    echo "       -t counts  Test rf on counts"
    exit 1;
fi

runName=$1

tr=5   # num of trees per machine
ma=20  # num of machines
num=$((tr*ma)) # total num of trees

mkdir -p $root/models/upenn/$runName
mkdir -p $root/exps/upenn/$runName
jids=""
for i in `seq 0 $tr $((num+tr))`; do
    jid="rf_train.$runName.$i"
    qsub -N $jid $root/scripts/rf_train.sh $runName $i $((i+tr-1)) \
	$smax $norand $simple $testrf
    jids="$jids,$jid"
done
qsub -hold_jid ${jids#,} $root/scripts/rf_ppl.sh $num $runName

exit 1;
