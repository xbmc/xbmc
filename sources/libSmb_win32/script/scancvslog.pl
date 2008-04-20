#!/usr/bin/perl
require"timelocal.pl";

#
# usage scancvslog.pl logfile starttime tag
#
# this will extract all entries from the specified cvs log file
# that have a date later than or equal to starttime and a tag
# value of tag. If starttime is not specified, all entries are
# extracted. If tag is not specified then entries for all
# branches are extracted. starttime must be specified as
# "monthname day, year"
#
# Example to extract all entries for SAMBA_2_2 branch from the
# log file named cvs.log 
#
# scancvslog.pl cvs.log "" SAMBA_2_2
#
#
# To extract all log entries after Jan 10, 1999 (Note month name
# must be spelled out completely).
#
# scancvslog.pl cvs.log "January 10, 1999" 
#

open(INFILE,@ARGV[0]) || die "Unable to open @ARGV[0]\n";

%Monthnum = (
	"January",	0,
	"February",	1,
	"March",	2,
	"April",	3,
	"May",		4,
	"June",		5,
	"July",		6,
	"August",	7,
	"September",	8,
	"October",	9,
	"November",	10,
	"December",	11,
	"Jan",		0,
	"Feb",		1,
	"Mar",		2,
	"Apr",		3,
	"May",		4,
	"Jun",		5,
	"Jul",		6,
	"Aug",		7,
	"Sep",		8,
	"Oct",		9,
	"Nov",		10,
	"Dec",		11
);

$Starttime = (@ARGV[1]) ? &make_time(@ARGV[1]) : 0;
$Tagvalue = @ARGV[2];

while (&get_entry) {
  $_=$Entry[0];
# get rid of extra white space
  s/\s+/ /g;
# get rid of any time string in date
  s/ \d\d:\d\d:\d\d/,/;
  s/^Date:\s*\w*\s*(\w*)\s*(\w*),\s*(\w*).*/$1 $2 $3/;
  $Testtime = &make_time($_);
  $Testtag = &get_tag;
  if (($Testtime >= $Starttime) && ($Tagvalue eq $Testtag)) {
    print join("\n",@Entry),"\n";
  }
}
close(INFILE);

sub make_time {
  $_ = @_[0];
  s/,//;
  ($month, $day, $year) = split(" ",$_);
  if (($year < 1900)||($day < 1)||($day > 31)||not length($Monthnum{$month})) {
    print "Bad date format @_[0]\n";
    print "Date needs to be specified as \"Monthname day, year\"\n";
    print "eg: \"January 10, 1999\"\n";
    exit 1;
  }
  $year = ($year == 19100) ? 2000 : $year;
  $month = $Monthnum{$month};
  $Mytime=&timelocal((0,0,0,$day,$month,$year));
}

sub get_tag {
  @Mytag = grep (/Tag:/,@Entry);
  $_ = @Mytag[0];
  s/^.*Tag:\s*(\w*).*/$1/;
  return $_;
}

sub get_entry {
  @Entry=();
  if (not eof(INFILE)) {
    while (not eof(INFILE)) {
      $_ = <INFILE>;
      chomp $_;
      next if (not ($_));
      if (/^\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*/) {
	next if ($#Entry == -1);
	push(Entry,$_);
	return @Entry;
      } else {
	push(Entry,$_);
      }
    }
  }
  return @Entry;
}
