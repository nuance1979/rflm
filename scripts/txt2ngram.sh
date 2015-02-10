#!/bin/bash
#
# Convert a text file into its counts in such a way 
#   that SRI LM Toolkit -counts .cnt would give exactly
#   the same result as -ppl .txt IN -T MODE!!!
# Otherwise get all the counts in reverse order
#   

sri=/home/yisu/Linux/srilm-1.4.5
sribin=$sri/bin/i686

order=3 # default tri-gram
tmode=0

while getopts "o:ht" opt; do
    case $opt in
	o ) order=$OPTARG;;
	t ) tmode=1;;
	* ) echo "$0: bad option $@"; exit 1;;
    esac
done
shift $(($OPTIND-1))

if [ $# -lt 2 ]; then
    echo "Usage: $0 [opts] txt cnt"
    echo " Opts: -o order       Max order of the counts (default: 3)"
    echo "       -t             Test mode"
    exit 1;
fi

txt=$1
cnt=$2

F=/tmp/$$

for i in `seq 1 $order`; do 
    args="$args -write$i $F.$i"
done

$sribin/ngram-count -order $order -sort -text $txt $args

if [ $tmode == "1" ]; then
    for i in `seq $((order-1)) -1 2`; do
	grep "^<s>" $F.$i >> $F.$order
    done
else
    for i in `seq $((order-1)) -1 1`; do
	cat $F.$i >> $F.$order
    done
fi

ct=${cnt%.gz}
if [ $ct == $cnt ]; then
    mv $F.$order $cnt
else
    gzip -f $F.$order
    mv $F.$order.gz $cnt
fi

rm $F.*
