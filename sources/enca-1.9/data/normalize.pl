#!/usr/bin/perl
use warnings;
use strict;

my $n = 0;
my $q;
my $max = 0;
my (@char, @count);

if (!defined $ARGV[0]) {
  while (<STDIN>) {
    ($char[$n], $count[$n]) = split /\s+/, $_, 2;

    if ($max < $count[$n]) {
      $max = $count[$n];
    }
    $n++;
  }

  for ($q = 1; $max/$q >= 6000; $q++) { }

  for (my $i = 0; $i < $n; $i++) {
    printf "%s %u\n", $char[$i], $count[$i]/$q;
  }
}
else {
  my @err;

  my $sum = 0;
  open FH, "<".$ARGV[0];
  while (<FH>) {
    my ($char, $count) = split /\s+/, $_, 2;
    $sum += $count;
  }
  close FH;

  my $sum2 = 0;
  while (<STDIN>) {
    ($char[$n], $count[$n]) = split /\s+/, $_, 2;

    $sum2 += $count[$n];
    $n++;
  }
  for (my $i = 0; $i < $n; $i++) {
    my $x = $count[$i]*$sum/$sum2;
    $count[$i] = int $x;
    $err[$i] = $count[$i] - $x;
  }
  $sum2 = 0;
  for (my $i = 0; $i < $n; $i++) {
    $sum2 += $count[$i];
  }

  while ($sum2 != $sum) {
    $max = 0;
    for (my $i = 0; $i < $n; $i++) {
      if ($err[$i]*($sum - $sum2) > $err[$max]*($sum - $sum2)) {
        $max = $i;
      }
    }
    $count[$max] += $sum2 > $sum ? -1 : 1;
    $err[$max] -= $sum2 > $sum ? -1 : 1;
    $sum2 += $sum2 > $sum ? -1 : 1;
  }

  for (my $i = 0; $i < $n; $i++) {
    printf "%s %u\n", $char[$i], $count[$i];
  }
}
