#!/usr/bin/perl
use warnings;
use strict;

my @w1 = ( 0 ) x 0x100;
my @w2 = ( 0 ) x 0x100;
my $m;

open FH, "<".$ARGV[0];
while (<FH>) {
  my $ch = substr $_,0,1;
  my $count = substr $_,2;
  $w1[ord $ch] = $count;
}
close FH;

open FH, "<".$ARGV[1];
while (<FH>) {
  my $ch = substr $_,0,1;
  my $count = substr $_,2;
  $w2[ord $ch] = $count;
}
close FH;

for (my $i = 0; $i < 0x100; $i++) {
  my $x = $w1[$i];
  $w1[$i] -= 4*$w2[$i];
  $w2[$i] -= 4*$x;
}

$m = 1;
while ($m > 0) {
  my $j = 0;
  for (my $i = 1; $i < 0x100; $i++) {
    if ($w1[$i] > $w1[$j]) {
      $j = $i;
    }
  }
  $m = $w1[$j];
  if ($m > 0) {
    printf '0x%02x, (%u)'."\n", $j, $m;
    $w1[$j] = -1;
  }
}

print "\n";

$m = 1;
while ($m > 0) {
  my $j = 0;
  for (my $i = 1; $i < 0x100; $i++) {
    if ($w2[$i] > $w2[$j]) {
      $j = $i;
    }
  }
  $m = $w2[$j];
  if ($m > 0) {
    printf '0x%02x, (%u)'."\n", $j, $m;
    $w2[$j] = -1;
  }
}
