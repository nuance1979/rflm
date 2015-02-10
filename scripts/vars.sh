#!/bin/bash

# Path
sri=/home/yisu/Linux/srilm-1.5.7
tmp=/tmp

# Global read-only 
root=/home/yisu/Work/rflm-dev
gtmp=$root/tmp
vocab=$root/dicts/upenn.10k.vocab

# Script
rfngram_count=$sri/rflm/bin/i686/rfngram-count
rfngram=$sri/rflm/bin/i686/rfngram

# Misc
ulimit -c 0 # disable core dump 
