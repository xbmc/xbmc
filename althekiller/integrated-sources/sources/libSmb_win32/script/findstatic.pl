#!/usr/bin/perl -w
# find a list of fns and variables in the code that could be static
# usually called with something like this:
#    findstatic.pl `find . -name "*.o"`
# Andrew Tridgell <tridge@samba.org>

use strict;

# use nm to find the symbols
my($saved_delim) = $/;
undef $/;
my($syms) = `nm -o @ARGV`;
$/ = $saved_delim;

my(@lines) = split(/\n/s, $syms);

my(%def);
my(%undef);
my(%stype);

my(%typemap) = (
	       "T" => "function",
	       "C" => "uninitialised variable",
	       "D" => "initialised variable"
		);


# parse the symbols into defined and undefined 
for (my($i)=0; $i <= $#{@lines}; $i++) {
	my($line) = $lines[$i];
	if ($line =~ /(.*):[a-f0-9]* ([TCD]) (.*)/) {
		my($fname) = $1;
		my($symbol) = $3;
		push(@{$def{$fname}}, $symbol);
		$stype{$symbol} = $2;
	}
	if ($line =~ /(.*):\s* U (.*)/) {
		my($fname) = $1;
		my($symbol) = $2;
		push(@{$undef{$fname}}, $symbol);
	}
}

# look for defined symbols that are never referenced outside the place they 
# are defined
foreach my $f (keys %def) {
	print "Checking $f\n";
	my($found_one) = 0;
	foreach my $s (@{$def{$f}}) {
		my($found) = 0;
		foreach my $f2 (keys %undef) {
			if ($f2 ne $f) {
				foreach my $s2 (@{$undef{$f2}}) {
					if ($s2 eq $s) {
						$found = 1;
						$found_one = 1;
					}
				}
			}
		}
		if ($found == 0) {
			my($t) = $typemap{$stype{$s}};
			print "  '$s' is unique to $f  ($t)\n";
		}
	}
	if ($found_one == 0) {
		print "  all symbols in '$f' are unused (main program?)\n";
	}
}

