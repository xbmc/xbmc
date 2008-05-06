#!/usr/bin/perl
# Generate a C flag-table from a list of bytes for which the flag should be on.
# Accepts either
# - a quoted string, character ordinal values are then used
# - a (reasonably separated) list of decimal or hexadecimal numbers
# Arguments:
# 1st  -- name of the flag file, table name is its basename with .t or .ti
#         removed, when the extension is .ti, the table is incremental
use strict;
use warnings;

my $sep = "[[:space:][:punct:]]";
my $dig = "([[:digit:]]{1,3}|0x[[:xdigit:]]{1,2})";
my @a = (0) x 0x100;
die "Need a filename" if not defined $ARGV[0];
my $incr = ($ARGV[0] =~ /\.ti$/) ? 1 : undef;
my $name = $ARGV[0];
$name =~ s/.*\///;
$name =~ s/\.ti?$//;

undef $/;
open FH, '<', $ARGV[0] or die "Cannot open $ARGV[0]\n";
$_ = <FH>;
close FH;
s/^[[:space:]]+//;
s/[[:space:]]+$//;
if (/"[^"]+"/) {
  s/"//g;
  foreach my $x (split / */) {
    $a[ord $x] = (defined $incr ? $incr++ : 1);
  }
}
elsif (/^($dig$sep+)+$dig?/s) {
  foreach my $x (split /$sep+/) {
    $a[$x] = (defined $incr ? $incr++ : 1);
  }
}
else {
  die "Cannot parse input string\n";
}

print "/* THIS IS A GENERATED TABLE, see tools/expand_table.pl */\n";
if ($incr) {
  print "static const short int $name\[\] = {\n";
}
else {
  print "static const unsigned char $name\[\] = {\n";
}
for (my $i = 0x00; $i < 0x100; $i++) {
  if ($i % 0x10 == 0x00) { print "  "; }
  if (defined $incr) { printf "%2d, ", $a[$i]; }
  else { print $a[$i] . ", "; }
  if ($i % 0x10 == 0x0f) { print "\n"; }
}
print "};\n";
