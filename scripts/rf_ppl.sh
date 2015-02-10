#!/bin/bash
#$ -S /bin/sh
#$ -e /home/yisu/Work/rflm-dev/err
#$ -o /home/yisu/Work/rflm-dev/out

source /home/yisu/Work/rflm-dev/scripts/vars.sh
source /home/yisu/Work/rflm-dev/scripts/functs.sh

export PATH=$sri/bin:$sri/bin/i686:$PATH

####

num=$1
runName=$2

exp=$root/exps/upenn/$runName
order=3
vocab=$root/dicts/upenn.10k.vocab
test=$root/data/upenn/counts/counts.23-24.3gm.gz
out=$exp/ppl

pplRF $num $exp $order $vocab $test $out

exit 1;

