#!/bin/bash
#$ -S /bin/sh
#$ -e /home/yisu/Work/rflm-dev/err
#$ -o /home/yisu/Work/rflm-dev/out

source /home/yisu/Work/rflm-dev/scripts/vars.sh
source /home/yisu/Work/rflm-dev/scripts/functs.sh

export PATH=$sri/bin:$sri/bin/i686:$PATH

####

runName=$1
start=$2
end=$3
smax=$4 # shallow tree mode
norand=$5 # no random selection
simple=$6
testrf=$7

order=3
in_order=3
vocab=$root/dicts/upenn.10k.vocab
train=$root/data/upenn/counts/counts.00-20.3gm.gz
tree=$root/models/upenn/$runName
backoff=$root/models/upenn/kn.3gm/lm.kn.3gm.gz
heldout=$root/data/upenn/counts/counts.21-22.3gm.gz
dist=$root/models/upenn/kn.3gm/discount

if [ $testrf ]; then 
    test=$testrf
else
    test=$root/data/upenn/counts/counts.23-24.3gm.gz
fi
out=$root/exps/upenn/$runName

for j in `seq $start $end`; do 
    if [ $j -lt 10 ]; then 
        i="00$j";
    else if [ $j -lt 100 ]; then
        i="0$j";
    else i=$j; fi; fi

    seed=$((j+3128))

    if [ $simple -eq 1 ]; then 
	if [ -z $testrf ]; then
	    trainRF_simple $order $vocab $train $tree/dt.$i.gz \
		$backoff $heldout $dist $seed \
		2> $tree/log.$i
	fi

	evalRF_simple $order $vocab $train $tree/dt.$i.gz \
	    $backoff $heldout $test $dist $out/lm.$i.gz \
	    2> $out/log.$i
    else
	if [ -z $testrf ]; then
	    trainRF $order $in_order $vocab $train $tree/dt.$i.gz \
		$backoff $heldout $dist $seed $smax $norand \
		2> $tree/log.$i
	fi

	evalRF $order $in_order $vocab $train $heldout $tree/dt.$i.gz \
	    $backoff $test $dist $out/lm.$i.gz \
	    2> $out/log.$i
    fi
done

exit 1;
