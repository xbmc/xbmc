#!/usr/bin/perl
use warnings;
use strict;

my ($total, @significant);
@significant = ( 0 ) x 0x100;

foreach my $name (@ARGV) {
  open FH, "<$name.c";
  local $/ = undef;
  my $file = <FH>;
  close FH;

  $file =~ s/\/\*.*?\*\///sg;
  $file =~ s/^[[:space:]]*[[:alnum:]_ ]+\[\] = {[[:space:]]*//s;
  $file =~ s/,[[:space:]]}[[:space:]]*;[[:space:]]*$//s;
  my @data = split /[[:space:],]+/, $file;

  my $w = 0;
  for (my $i = 0; $i < 0x100; $i++) {
    $w += $data[$i];
    $significant[$i] += $data[$i];
  }
  die "$name: $w (expecting $total)\n" if defined $total && $total != $w;
  $total = $w;
}

open FH, ">totals.c";
print FH "/* THIS IS A GENERATED TABLE, see data/totals.pl. */\n";
print FH "static const unsigned short int SIGNIFICANT[] = {\n";
for (my $i = 0; $i < 0x100; $i++) {
  if ($i % 8 == 0) {
    printf FH "  ";
  }
  printf FH "%5u", $significant[$i];
  if ($i % 8 == 7) {
    printf FH ",  /* 0x%02x */\n", $i-7;
  }
  else {
    print FH ", ";
  }
}
print FH "};\n\n";

print FH "/* THIS IS A GENERATED VALUE, see data/totals.pl */\n";
print FH "#define WEIGHT_SUM $total\n\n";

print FH "/* THIS IS A GENERATED TABLE, see data/totals.pl */\n";
print FH "static const char *const CHARSET_NAMES[] = {\n";
foreach my $name (@ARGV) {
  print FH "  \"$name\",\n";
}
print FH "};\n\n";

print FH "/* THIS IS A GENERATED TABLE, see data/totals.pl */\n";
print FH "static const unsigned short int *const CHARSET_WEIGHTS[] = {\n";
foreach my $name (@ARGV) {
  my $raw = "RAW_" . uc $name;
  print FH "  $raw,\n";
}
print FH "};\n\n";

if (<*.p.c>) {
  print FH "/* THIS IS A GENERATED TABLE, see data/totals.pl */\n";
  print FH "static const unsigned char *const CHARSET_LETTERS[] = {\n";
  foreach my $name (@ARGV) {
    my $raw = "LETTER_" . uc $name;
    print FH "  $raw,\n";
  }
  print FH "};\n\n";

  print FH "/* THIS IS A GENERATED TABLE, see data/totals.pl */\n";
  print FH "static const unsigned char **const CHARSET_PAIRS[] = {\n";
  foreach my $name (@ARGV) {
    my $raw = "PAIR_" . uc $name;
    print FH "  $raw,\n";
  }
  print FH "};\n\n";
}
else {
  print FH "/* THIS IS A GENERATED VALUE, see data/totals.pl */\n";
  print FH "#define CHARSET_LETTERS NULL\n\n";

  print FH "/* THIS IS A GENERATED VALUE, see data/totals.pl */\n";
  print FH "#define CHARSET_PAIRS NULL\n\n";
}

print FH "/* THIS IS A GENERATED VALUE, see data/totals.pl */\n";
print FH "#define NCHARSETS " . ($#ARGV + 1) . "\n";

close FH;
