#!/usr/bin/perl -w
# Turns binary files into source code of initialized arrays.
use strict;
for (@ARGV) {
	open INPUT, $_ and binmode INPUT or die "$_: $!\n";
	s!.*[/\\]!!;
	y/0-9A-Za-z/_/c;
	print "CONST_ARRAY(byte, $_)\n\t";
	$/ = \1;
	$. = 0;
	print $. % 16 != 1 ? ',' : $. > 1 ? ",\n\t" : '', ord
		while <INPUT>;
	print "\nEND_CONST_ARRAY;\n"
}
