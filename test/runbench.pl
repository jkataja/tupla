#!/usr/bin/perl
#
# Benchmarking script for running suffix array sorting
#
# @author jkataja

use strict;
use warnings;
use Time::HiRes;

# More readable time display
sub pretty_time {
	my ($time) = @_;
	if ($time < 0.1) { return "< 0.1s"; }
	if ($time < 10) { return sprintf("%.1f", $time)."s"; }
	if ($time < 60) { return int($time)."s"; }
	if ($time < 600) { return sprintf("%.1f", $time/60)."s"; }
	return int($time/60)."m";
}

sub run_timed {
	my ($cmd, $runs) = @_;
	$runs = 1 unless defined $runs;
	$runs *= 1.0;
	my $wall = 0.0;
	my $loadavg = `cat /proc/loadavg | cut -f1,2,3 -d' '`; # Linux only
	chomp $loadavg;
	print STDERR "*** Command '$cmd'\n";
	print STDERR "*** Averaging runtime over $runs runs\n";
	print STDERR "*** Load: $loadavg\n";
	for my $r (1..$runs) {
		my $start = [ Time::HiRes::gettimeofday( ) ];
		system($cmd);
		die "Command '$cmd' returned error\n" if ($? != 0 );
		my $time = Time::HiRes::tv_interval( $start );
		$wall += $time;
		print STDERR "*** Run $r timed: ".&pretty_time($time)."\n";
	}
	my $avgtime = ($wall / $runs); # average
	print STDERR "*** Averaged time ".&pretty_time($avgtime)."\n";
	return $avgtime;
}

my $path  = shift;
die "Usage: $0 in/path/\n" unless defined $path && -d $path; 

my %files;
my %runtimes;

opendir(DIR, $path) or die $!;
while (my $file = readdir(DIR)) {
	next if $file =~ /^\./; # starts with .
	next if $file =~ /\.(rank|lcp)$/; # working files
	next if $file eq 'md5sums' || $file eq 'README'; # cruft
	$files{$file}++;
}

die "No input files found in path\n" unless scalar keys %files;

my $bin = "bin/tupla";
my $baseopts = "-b"; # don't write output

my @jobs = map { 2 ** $_ } (0..3);
my $runs = 1;
my @out;

# Header (Linux only)
print STDERR "*** Host: " . `hostname`;
print STDERR "*** CPU model:" . `grep -m 1  '^model name' /proc/cpuinfo | cut -f2 -d':'`;
print STDERR "*** CPU cache:" . `grep -m 1  '^cache size' /proc/cpuinfo | cut -f2 -d':'`;


# Input directory
foreach my $file (keys %files) {
	my $basetime = -1.0;
	foreach my $jobs (@jobs) {
		my $filepath = "$path/$file"; $filepath =~ s/\/\//\//g;
		my $cmd = "$bin $baseopts -j$jobs $filepath";
		my $avgtime;
		my $len = -s $filepath;
		if (defined $runtimes{$cmd}) { $avgtime = $runtimes{$cmd}; }
		else { $avgtime = &run_timed($cmd, $runs); }
		if ($basetime < 0) { $basetime = $avgtime; }
		my $ratio = ($basetime / $avgtime);
		push @out, sprintf "$file,%d,%d,%.2f,%.3f\n", $len,$jobs,$avgtime,$ratio;
	}
}

# Same file capped to different length
my $largetext = "data/largetext/enwik8";

foreach my $len ( map { 10 ** $_ } (5..8) ) {
	my $basetime = -1.0;
	foreach my $jobs (@jobs) {
		my $cmd = "$bin $baseopts -j$jobs -n$len $largetext";
		my $avgtime;
		if (defined $runtimes{$cmd}) { $avgtime = $runtimes{$cmd}; }
		else { $avgtime = &run_timed($cmd, $runs); }
		if ($basetime < 0) { $basetime = $avgtime; }
		my $ratio = ($basetime / $avgtime);
		push @out, sprintf "enwik8,%d,%d,%.2f,%.3f\n", $len,$jobs,$avgtime,$ratio;
	}
}

foreach my $line (@out) { print $line }