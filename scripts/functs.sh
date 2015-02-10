#!/bin/bash

function trainLM {
    local order=$1
    local vocab=$2
    local train=$3 # counts
    local dist=$4  # always(!) save discount parameters in files
    local lm=$5    # output lm
    local vocab1=$6

    local i
    local opts=""
    if [ $vocab1 ]; then 
        opts="-nonevents $vocab1"
    fi
    for i in `seq 1 $order`; do
	opts="$opts -kndiscount$i -kn$i $dist.$i -gt${i}min 1"
    done

    # building lm
    # 
    # NOTE: "-gt3min 1" is crucial to get the same ppl number as Peng!!! 
    #
    #       Generally, I found low cutoff was good for small data set like
    #       Upenn treebank but bad for huge data set like Gigaword.
    #

    # estimate discount parameters
    ngram-count -vocab $vocab -unk \
	-order $order \
	-read $train \
	-interpolate \
	$opts

    # estimate lm
    ngram-count -vocab $vocab -unk \
	-order $order \
	-read $train \
	-interpolate \
	-lm $lm \
	$opts
}

function trainLM_big {
    local order=$1
    local vocab=$2
    local train=$3 # counts
    local dist=$4  # always(!) save discount parameters in files
    local lm=$5    # output lm

    local i
    local opts=""
    for i in `seq 1 $order`; do
	opts="$opts -kndiscount$i"
    done

    # building lm
    local F=$tmp/$$

    mkdir -p $F
    make-big-lm -name $F/biglm \
	-order $order \
	-read $train \
	-vocab $vocab -unk \
	-lm $lm \
	-interpolate \
	-prune 1e-8 \
	$opts

    for i in `seq 1 $order`; do
	mv $F/biglm.kn$i $dist.$i
    done

    rm -rf $F
}

function evalLM {
    local order=$1
    local vocab=$2
    local eval=$3 # eval counts
    local lm=$4
    local ppl=$5 # output ppl file
    local vocab1=$6

    local opts=""
    if [ $vocab1 ]; then
        opts="-nonevents $vocab1"
    fi

    ngram -vocab $vocab -unk \
	-order $order \
	-counts $eval \
	-lm $lm \
	$opts \
	> $ppl
}

function trainRF_simple {
    local order=$1 
    local vocab=$2
    local train=$3 # counts
    local tree=$4
    local backoff=$5
    local heldout=$6
    local dist=$7 # prefix of discount files
    local seed=$8

    local i
    local opts=""
    for i in `seq 1 $order`; do
#	opts="$opts -ukndiscount$i -kn$i $dist.$i -gt${i}min 1"
	opts="$opts -kndiscount$i -kn$i $dist.$i -gt${i}min 1"
    done

    $rfngram_count -vocab $vocab -unk \
	-order $order \
	-read $train \
	-dt $tree \
	-backoff-lm $backoff \
	-heldout-read $heldout \
	-interpolate \
	-seed $seed \
	$opts
}

function trainRF_big {
    local order=$1
    local vocab=$2
    local train=$3 # counts
    local tree=$4
    local backoff=$5
    local heldout=$6 # counts, not text
    local dist=$7

    local i
    local opts=""
    for i in `seq 1 $order`; do
#	opts="$opts -ukndiscount$i -kn$i $dist.$i"
	opts="$opts -kndiscount$i -kn$i $dist.$i"
    done

    local F=$tmp/$$
    local dt_counts="$F.counts"
    local dt_heldout="$F.heldout"
    local dt_tree="$F.tree"

    $rfngram_count -vocab $vocab -unk \
	-order $order \
	-read $train -read-with-mincounts \
	-dt $dt_tree \
	-dt-counts $dt_counts \
	-dt-heldout $dt_heldout \
	-heldout-read $heldout \
	-backoff-lm $backoff \
	-interpolate \
	-dt-max-nodes 3000 \
	$opts

    local tr=${tree%.gz}
    if [ $tr == $tree ]; then
	mv $dt_tree $tree
    else
	mv $dt_tree $tr
	gzip -f $tr
    fi
    
    rm -f $dt_counts.* $dt_heldout.*
    rm -f $dt_tree.*
}

function trainRF {
    local order=$1
    local in_order=$2
    local vocab=$3
    local train=$4 # counts
    local tree=$5
    local backoff=$6
    local heldout=$7 # counts, not text
    local dist=$8
    local seed=$9
    local shallow=${10:-0} # max nodes for shallow tree if > 0
    local norand=${11:-0} # flag to turn off random selection
    local vocab1=${12}

    local i
    local opts=""
    for i in `seq 1 $in_order`; do
	opts="$opts -kndiscount$i -kn$i $dist.$i -gt${i}min 1"
    done

    local maxnodes=300000000
    if [ $shallow -gt 0 ]; then
        opts="$opts -shallow-tree"
        maxnodes=$shallow
    fi

    if [ $norand -ne 0 ]; then
#        opts="$opts -no-random"
	opts="$opts -random-select-only"
    fi

    if [ $vocab1 ]; then
        opts="$opts -nonevents $vocab1"
    fi

    local F=$tmp/$$
    local dt_counts="$F.counts"
    local dt_heldout="$F.heldout"
    local dt_tree="$F.tree"

    $rfngram_count -vocab $vocab -unk \
	-order $order \
	-internal-order $in_order \
	-read $train \
	-dt $dt_tree \
	-dt-counts $dt_counts \
	-dt-heldout $dt_heldout \
	-heldout-read $heldout \
	-backoff-lm $backoff \
	-interpolate \
	-dt-max-nodes $maxnodes \
	-seed $seed \
	-debug 1 \
	$opts

    local tr=${tree%.gz}

    if [ $tr == $tree ]; then
	mv $dt_tree $tree
    else
	mv $dt_tree $tr
	gzip -f $tr
    fi
    
    rm -f $dt_counts.* $dt_heldout.*
    rm -f $dt_tree.*
}

function evalRF_simple {
    local order=$1
    local vocab=$2
    local train=$3 # counts including check
    local tree=$4
    local backoff=$5
    local heldout=$6
    local test=$7 # test counts 
    local dist=$8 # discount parameter files 
    local output=$9

    local i
#    local opts="-ukndiscount"
    for i in `seq 1 $order`; do
	opts="$opts -kn$i $dist.$i"
    done

    $rfngram -vocab $vocab -unk \
	-order $order \
	-read $train \
	-heldout-read $heldout \
	-dt $tree \
	-lm $backoff \
	-write-lm $output \
	-counts $test \
	$opts
}

function evalRF {
    local order=$1
    local in_order=$2
    local vocab=$3
    local train=$4 # counts not including heldout
    local heldout=$5 # counts, not text
    local tree=$6
    local backoff=$7
    local test=$8 # test counts
    local dist=$9 # discount parameter files
    local output=${10} # ${10} != $10 !!!!!!!!!!!!!!!!!!!!!!!!!
    local vocab1=${11}

    local i
    local opts=""
    for i in `seq 1 $in_order`; do
	opts="$opts -kn$i $dist.$i"
    done

    if [ $vocab1 ]; then
        opts="$opts -nonevents $vocab1"
    fi

    local F=$tmp/$$
    local dt_counts="$F.stats"
    local dt_test="$F.test"

    $rfngram -vocab $vocab -unk \
	-order $order \
	-internal-order $in_order \
	-read $train \
	-heldout-read $heldout \
	-dt $tree \
	-dt-counts $dt_counts \
	-dt-test $dt_test \
	-lm $backoff \
	-write-lm $output \
	-counts $test \
	-debug 1 \
	$opts

    rm -f $dt_counts.* $dt_test.*
}

function pplRF {
    local num=$1
    local dir=$2
    local order=$3
    local vocab=$4
    local test=$5
    local out=$6 # output ppl file
    local vocab1=$7

    local F=/tmp/$$
    ls $dir/lm.???.gz > $F

    local opts=""
    if [ $vocab1 ]; then
        opts="-nonevents $vocab1"
    fi

    $rfngram -vocab $vocab -unk \
	-order $order \
	-mix-num $num \
	-mix-list $F \
	-write-lm $dir/lm.gz \
        $opts

    ngram -vocab $vocab -unk \
	-order $order \
	-lm $dir/lm.gz \
	-counts $test \
        $opts \
	> $out

    rm -f $F
}

function startServer {
    local order=$1
    local vocab=$2
    local backoff=$3
    local port=$4

    $rfngram -vocab $vocab -unk \
	-order $order \
	-lm $backoff \
	-port $port \
	-server &
}

function trainRF_client {
    local order=$1
    local vocab=$2
    local train=$3 # counts
    local tree=$4
    local port=$5 # backoff lm server port
    local heldout=$6 # counts, not text
    local dist=$7

    local i
    local opts=""
    for i in `seq 1 $order`; do
#	opts="$opts -ukndiscount$i -kn$i $dist.$i"
	opts="$opts -kndiscount$i -kn$i $dist.$i"
    done

    local F=$tmp/$$
    local dt_counts="$F.counts"
    local dt_heldout="$F.heldout"
    local dt_tree="$F.tree"

    $rfngram_count -vocab $vocab -unk \
	-order $order \
	-read $train -read-with-mincounts \
	-dt $dt_tree \
	-dt-counts $dt_counts \
	-dt-heldout $dt_heldout \
	-heldout-read $heldout \
	-backoff-host 127.0.0.1 \
	-backoff-port $port \
	-interpolate \
	-dt-max-nodes 2000 \
	$opts

    local tr=${tree%.gz}
    if [ $tr == $tree ]; then
	mv $dt_tree $tree
    else
	mv $dt_tree $tr
	gzip -f $tr
    fi
    
    rm -f $dt_counts.* $dt_heldout.*
    rm -f $dt_tree.*
}

function evalRF_big {
    local order=$1
    local vocab=$2
    local train=$3 # counts not including heldout
    local heldout=$4 # counts, not text
    local tree=$5
    local backoff=$6
    local test=$7 # test text
    local dist=$8 # discount parameter files
    local output=$9

    local i
    local opts=""
    for i in `seq 1 $order`; do
	opts="$opts -kn$i $dist.$i"
    done

    local F=$tmp/$$
    local dt_counts="$F.stats"
    local dt_test="$F.test"

    $rfngram -vocab $vocab -unk \
	-order $order \
	-read $train -read-with-mincounts \
	-heldout-read $heldout \
	-dt $tree \
	-dt-counts $dt_counts \
	-dt-test $dt_test \
	-lm $backoff \
	-write-lm $output \
	-ppl $test \
	$opts

    rm -f $dt_counts.* $dt_test.*
}

function interpLM {
    local lm1=$1
    local lm2=$2
    local held=$3 # heldout text for estimating interp weights
    local test=$4 # test text
    local out=$5

    local F=$tmp/$$

    ngram -lm $lm1 -unk -ppl $held -debug 2 > $F.1
    ngram -lm $lm2 -unk -ppl $held -debug 2 > $F.2

    compute-best-mix $F.1 $F.2 > $F.weights
    
    local lambda=`cat $F.weights | perl -ane'$t=$F[@F-2]; $t =~ s/\(//; print $t'`
    
    tail -2 $F.1 > $out
    tail -2 $F.2 >> $out
    cat $F.weights >> $out
    ngram -lm $lm1 -unk -mix-lm $lm2 -lambda $lambda -ppl $test >> $out

    rm -f $F.*
}
