#!/bin/bash
#$ -S /bin/sh
#$ -e /home/yisu/Work/rflm-dev/err
#$ -o /home/yisu/Work/rflm-dev/out

source /home/yisu/Work/rflm-dev/scripts/vars.sh
source /home/yisu/Work/rflm-dev/scripts/functs.sh

export PATH=$sri/bin:$sri/bin/i686/:$PATH

####

mkdir -p $root/models/upenn/kn.3gm
order=3
vocab=$root/dicts/upenn.10k.vocab
train=$root/data/upenn/counts/counts.00-22.3gm.gz
dist=$root/models/upenn/kn.3gm/discount
lm=$root/models/upenn/kn.3gm/lm.kn.3gm.gz

trainLM $order $vocab $train $dist $lm
exit 1;

lm1=$root/exps/upenn/baseline/lm.gz
lm2=$root/models/upenn/kn.3gm/lm.kn.3gm.gz
held=$root/data/upenn/text.23-24
test=$root/data/upenn/text.23-24
out=$root/exps/upenn/interp/baseline.out

interpLM $lm1 $lm2 $held $test $out
exit 1;
