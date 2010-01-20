#!/usr/bin/perl
# Archive::Tar doesn't give full control over what is stored in the archive,
# so I do not use it.
use strict;

my $dir_time;
my $top_input_dir = '';
my $top_tar_dir;

sub write_entry($$$$$) {
	my ($filename, $data, $type, $mode, $mtime) = @_;
	my $header = pack 'a100a8a8a8a12a12',
		$top_tar_dir . $filename,
		'0000' . $mode, '0000000', '0000000',
		sprintf('%011o', length($data)), sprintf('%011o', $mtime);
	print $header;
	printf '%06o', 256 + 48 + $type + unpack '%16C*', $header;
	print "\0 ", $type, v0 x 355, $data, v0 x (-length($data) & 511);
}

sub write_dir($$) {
	my ($sub_dir, $r) = @_;
	write_entry($sub_dir, '', 5, '755', $dir_time);
	for (grep !ref($r->{$_}), sort keys %$r) {
		my $filename = "$top_input_dir$sub_dir$_";
		my $type = $r->{$_};
		my $data;
		my $mtime;
		open INPUT, $filename
			and binmode INPUT
			and read INPUT, $data, 1_000_000
			and $mtime = (stat INPUT)[9]
			and close INPUT
			or die "maketar.pl: $filename: $!\n";
		$data =~ s/\r\n/\n/g if $type =~ y/ts//;
		write_entry($sub_dir . $_, $data, 0, $type =~ y/sx// ? '755' : '644', $mtime);
	}
	for (grep ref($r->{$_}), sort keys %$r) {
		write_dir("$sub_dir$_/", $r->{$_});
	}
}

if (@ARGV < 4 || $ARGV[0] ne '-d' || $ARGV[2] !~ /^-[tbsx]$/) {
	die "Usage: perl maketar.pl -d dir_in_tar -t text_files -b binary_files -s executable_text_files -x executable_binary_files\n";
}
shift;
$top_tar_dir = shift() . '/';
my $type;
my %input;
for (@ARGV) {
	if (/^-([tbsx])$/) {
		$type = $1;
		next;
	}
	$type or die "maketar.pl: $_: Unspecified file type\n";
	my $r = \%input;
	$r = \%{$r->{$1}} while m!(.+?)[/\\]!gc;
	/(.+)/g;
	$r->{$1} = $type;
}
my $r = \%input;
while (%$r == 1) {
	my ($k) = keys %$r;
	ref($r->{$k}) or last;
	$top_input_dir .= "$k/";
	$r = $r->{$k};
}
$dir_time = (stat '.')[9] or die "maketar.pl: Cannot stat '.'\n";
binmode STDOUT;
write_dir('', $r);
print "\0" x 1024;
