#!/usr/bin/perl
#
# Set up directory structure and install scripts
#

use strict;

if (@ARGV < 3) {
    print STDERR "Usage: $0 <work_dir> <sri_dir> <sri_mach_type>\n";
    print STDERR " Note: Use absolute paths for safe grid jobs.\n";
    exit(0);
}

my $work_dir = $ARGV[0];
my $sri_dir = $ARGV[1];
my $sri_mach_type = $ARGV[2];

if ($work_dir !~ /^\// || $sri_dir !~ /^\//) {
    print STDERR "ERROR: One of the paths is not an absolute path.\n";
    exit(0);
}

my $cmd;

$cmd = "mkdir $work_dir";
runCmd($cmd);

$cmd = "cd $work_dir; mkdir data dicts exps models scripts tmp";
runCmd($cmd);

$cmd = "cd $work_dir; mkdir -p data/upenn/counts models/upenn exps/upenn";
runCmd($cmd);

$cmd = "cp $sri_dir/rflm/scripts/functs.sh $work_dir/scripts/";
runCmd($cmd);

$cmd = "cp $sri_dir/rflm/scripts/txt2ngram.sh $work_dir/scripts/";
runCmd($cmd);

my @list = ("vars.sh", "run_rf.sh", "rf_train.sh", "rf_ppl.sh");
my $file;

foreach $file (@list) {
    open(IN, "$sri_dir/rflm/scripts/$file") or die;
    open(OUT, ">$work_dir/scripts/$file") or die;
    while (<IN>) {
	s,/home/yisu/Work/rflm-dev,$work_dir,;
	s,/home/yisu/Linux/srilm-1.5.7,$sri_dir,;
	s,i686,$sri_mach_type,;
	print OUT $_;
    }
    close(IN);
    close(OUT);
}

$cmd = "chmod 755 $work_dir/scripts/*";
runCmd($cmd);

#########

sub runCmd {
    my $cmd = shift;
    
    print STDERR "$cmd\n";
    system($cmd);
    if ($? == -1) {
	print STDERR "ERROR: Command failed: $!\n";
	exit(0);
    }
}
