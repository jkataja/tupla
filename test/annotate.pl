#!/usr/bin/perl
#
# Run a command and prefix each line of it's output with running time
# and output stream. Some lines in output may have changed order.
# Adapted from IPC::Open3::Simple
#
# @author jkataja

use strict;
use warnings;

use Time::HiRes qw/gettimeofday tv_interval/;
use IO::Select;
use IPC::Open3 qw/open3/;
use Symbol qw/gensym/;

my $t0; 

sub annotate {
	my ($tag, $line) = @_;
	printf "[ %.4f ] %s: %s", Time::HiRes::tv_interval($t0), $tag, $line;
}

die "usage: annotate.pl program [args...]\n" if $#ARGV < 0;

my $pid = open3(gensym, \*CHILD_OUT, \*CHILD_ERR, @ARGV);
$t0 = [Time::HiRes::gettimeofday];

&annotate("I", "Started '".join(" ",@ARGV). "'\n");

my $reader = IO::Select->new(\*CHILD_OUT, \*CHILD_ERR);
while ( my @ready = $reader->can_read() ) {
	foreach my $fh (@ready) {
		my $line = <$fh>;
		if (!defined $line) { $reader->remove($fh); next; }
		if (fileno($fh) == fileno(\*CHILD_OUT)) {
			&annotate("O", $line);
		} 
		elsif (fileno($fh) == fileno(\*CHILD_ERR)) {
			&annotate("E", $line);
		}
	}
}
waitpid($pid, 0);
&annotate("I", "Finished with exit code ".($? >> 8)."\n");

exit ($? >> 8);

# vim:set ts=4 sts=4 sw=4 noexpandtab:
