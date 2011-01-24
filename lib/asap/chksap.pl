#!/usr/bin/perl

=head1 NAME

chksap - check, fix or calculate statistics for *.sap files

=head1 SYNOPSIS

  perl chksap.pl option... file...

=head1 DESCRIPTION

I<chksap> processes SAP (Slight Atari Player) files.
It can operate in one of three modes: check file format conformance,
fix errors automatically, compute statistics.

You must select a mode using one of the options described below
and pass names of files or directories to be processed.
Directories are processed recursively, but only C<*.sap> files
are analyzed.

=head1 OPTIONS

Exactly one of the following options should be used to select
the operation:

=over 4

=item B<-c>, B<--check>

Display the errors found for each file.  Does not display anything
for correct files.

=item B<-f>, B<--fix>

Try to automatically fix errors.  Shows descriptions for the fixed
errors and the errors that cannot be fixed automatically.

B<Warning:> files are modified in-place and no backup copies are made.
Make backups yourself before using this option!

=item B<-s>, B<--statistics>

Compute and display statistics.

=back

The following options are available in any mode:

=over 4

=item B<-p>, B<--progress>

Print name of the currently processed file.

=back

In the fix mode the following options are available:

=over 4

=item B<-t>, B<--time>

For files that have no TIME tag detects playback time using an external
program "asapscan" and stores it in TIME tags.

=item B<-T>, B<--overwrite-time>

Detects playback time using an external program "asapscan" and stores it
in TIME tags, possibly overwriting the current values.

=back

In the statistics mode the following options are available:

=over 4

=item B<-t>, B<--time>

Lists files with no TIME tags.

=item B<-u>, B<--features>

Lists files with rarely used POKEY and 6502 features.

=back

The following options display some text and terminate the program
without file operations:

=over 4

=item B<-h>, B<--help>

Display usage summary.

=item B<-v>, B<--version>

Display program version.

=back

=head1 ERRORS FIXED AUTOMATICALLY

=over 4

=item B<line terminators within header are not CRLF>

The lines of the header should be separated with CR/LF pairs
and not just LF or CR characters.

=item B<header not terminated with CRLF>

The last line of the header is not terminated with CR/LF.

=item B<extra whitespace within header>

There should be exactly one space between the tag name
and its argument. There should be no spaces after the argument.

=item B<missing AUTHOR, NAME or DATE tag>

Although AUTHOR, NAME and DATE have no meaning to SAP players,
they should always appear in the file.  If the value is unknown,
C<<"<?>">> should be used.

=item B<blank AUTHOR, NAME or DATE argument>

If the value is unknown, it should not be empty or consist
of only question marks - use C<<"<?>">> instead.

=item B<SONGS 1 is superfluous>

C<SONGS> tag with the argument of 1 is meaningless
because the number of songs defaults to 1.

=item B<DEFSONG 0 is superfluous>

C<DEFSONG> tag with the argument of 0 is meaningless
because the default song defaults to 0.

=item B<hexadecimal values should be 4-digit>

The arguments of INIT, PLAYER, MUSIC and COVOX should specify all 4 digits.

=item B<hexadecimal values should be uppercase>

The arguments of INIT, PLAYER, MUSIC and COVOX should use C<A-F> letters
rather than C<a-f>.

=item B<non-standard order of tags>

The order of tags is of little importance, but making some assumptions
about it makes it easier to create new tools that process SAP files.
The canonical order is: first SAP, then AUTHOR, then NAME, then DATE.
After DATE, the following tags should be used in any order: SONGS, TYPE,
FASTPLAY and STEREO.  DEFSONG must appear after SONGS.
The tags that should appear last are: INIT, PLAYER, MUSIC and COVOX,
in any order.

=item B<FFFF inside binary part>

Two C<0xFF> bytes start the binary part but are redundant
inside it.

=item B<garbage bytes at the end>

There are extra bytes after the last binary block, but less than 4 necessary
to form a new block header.

=item B<truncated binary block>

There are not enough bytes in the file to fill the last binary block.

=back

=head1 ERRORS THAT CANNOT BE FIXED AUTOMATICALLY

=over 4

=item B<filename too long>

For portability, filenames should be limited to 26 characters
(plus C<.sap>).

=item B<illegal characters in filename>

Filenames should consist of letters (C<A-Z> and C<a-z>),
digits (C<0-9>) and underscore characters (C<_>).

=item B<file too long>

File should not exceed 63 KB (64512 bytes).  This is because only 62 KB
base RAM is available for use in Atari XL/XE machines (2 KB is reserved
for I/O area) and the SAP header should be no longer than 1 KB.

=item B<missing SAP header>

The file does not begin with C<SAP>.

=item B<missing binary part>

Two C<0xFF> bytes were not found in the file.

=item B<unknown header line: ...>

A line of the header does not start with an uppercase tag name.

=item B<unexpected argument of SAP or STEREO>

SAP and STEREO tags should have no argument.

=item B<invalid argument of AUTHOR, NAME or DATE>

The arguments of these tags should be wrapped in double quotes
and should consist of ASCII characters between space and lowercase 'z',
except for the double quote and backtick (0x60) characters.
Moreover, no argument should be longer than 122 characters
(including quotes).

=item B<invalid argument of SONGS>

The argument of SONGS must be an integer within the range 2-255.

=item B<invalid argument of DEFSONG>

The argument of DEFSONG must be an integer greater than zero
and less than the argument of SONGS.

=item B<missing TYPE or PLAYER tag>

These tags are mandatory.

=item B<invalid argument of TYPE>

The currently defined types are: B, C, D, S.

=item B<invalid argument of FASTPLAY>

The argument of FASTPLAY must be an integer within the range 1-311.

=item B<INIT is meaningless with TYPE C>

C<TYPE C> uses PLAYER and MUSIC tags, but not INIT.

=item B<MUSIC is meaningful only with TYPE C>

The MUSIC tag should be used only with C<TYPE C>.

=item B<invalid argument of INIT, PLAYER, MUSIC or COVOX>

The argument of these tags must be 4-digit hexadecimal number.

=item B<duplicate tag: ...>

The same tag appears several times within the header.

=item B<unknown tag: ...>

Known tags are: SAP, AUTHOR, NAME, DATE, SONGS, DEFSONG, TYPE, FASTPLAY,
STEREO, INIT, PLAYER, MUSIC, COVOX.

=item B<duplicate FFFF in the binary part>

A binary block starts with four C<0xFF> bytes.

=item B<block end address less than start address>

Each binary block begins with a header that specifies the addresses
of the first and the last byte in the block. The end address must not
be less than the start address.

=back

=head1 BUGS

The checks are more strict than necessary, which is a good thing.

=head1 AUTHOR

Piotr Fusik C<fox@scene.pl>.

=cut

use File::Find;
use File::Spec;
use Getopt::Long;
use Pod::Usage;
use strict;

my $VERSION = '1.2.1';
my $asapscan = File::Spec->rel2abs('asapscan');
my ($check, $fix, $stat) = (0, 0, 0);
my ($progress, $time, $overwrite_time, $features, $help, $version) = (0, 0, 0, 0, 0, 0);
my ($total_files, $sap_files, $stereo_files) = (0, 0, 0);
my ($total_length, $min_length, $max_length) = (0, 100_000, 0);
my ($min_filename, $max_filename);
my ($time_files, $total_millis) = (0, 0);
my (@fixed_messages, @notfixed_messages, @fatal_messages, @no_time_files);
my (%stat, %types, %features);

sub process($$) {
	my ($filename, $fullpath) = @_;
	my (%fixed, %fatal);
	my $sap;
	print STDERR "$fullpath\n" if $progress;
	open F, '<', $filename and binmode F and read F, $sap, 64513 and close F
		or die "$filename: $!\n";
	{
		++$total_files;
		my $len = length($sap);
		if ($len > 64512) {
			$fatal{'file too long'} = 1;
			last;
		}
		$fatal{'filename too long'} = 1 if length($filename) > 30;
		if ($filename !~ /^\w+\.sap$/is) {
			$fatal{'illegal characters in filename'} = 1;
		}
		if ($sap !~ /^SAP\W/s) {
			$fatal{'missing SAP header'} = 1;
			last;
		}
		if ($sap !~ /^(.+?)\xFF\xFF(.{5,})$/s) {
			$fatal{'missing binary part'} = 1;
			last;
		}
		++$sap_files;
		$total_length += $len;
		if ($min_length > $len) {
			$min_length = $len;
			$min_filename = $filename;
		}
		if ($max_length < $len) {
			$max_length = $len;
			$max_filename = $filename;
		}
		my ($hdr, $bin) = ($1, $2);
		if ($hdr =~ /\x0D[^\x0A]|[^\x0D]\x0A/) {
			$fixed{'line terminators within header are not CRLF'} = 1;
		}
		if ($hdr !~ s/\x0D\x0A$//s) {
			$fixed{'header not terminated with CRLF'} = 1;
		}
		my %tags;
		my @times;
		for (split /[\x0D\x0A]+/, $hdr) {
			my ($tag, $spaces1, $arg, $spaces2);
			unless (($tag, $spaces1, $arg, $spaces2) =
				/^([A-Z]+)(?:( +)(.+?))?( *)$/s) {
				$fatal{"unknown header line: $_"} = 1;
				next;
			}
			if (length($spaces1) > 1 || $spaces2) {
				$fixed{'extra whitespace within header'} = 1;
			}
			if (exists($tags{$tag})) {
				$fatal{"duplicate tag: $tag"} = 1;
				next;
			}
			if ($tag eq 'SAP') {
				$fatal{'unexpected argument of SAP'} = 1 if $arg ne '';
			}
			elsif ($tag eq 'AUTHOR') {
				++$stat{'AUTHOR'}{$arg};
				if ($arg !~ /^"[ !#-_a-z]{0,120}"$/s) {
					$fatal{'invalid argument of AUTHOR'} = 1;
				}
				elsif ($arg =~ /^"\?*"$/s) {
					$fixed{'blank AUTHOR argument'} = 1;
					$arg = '"<?>"';
				}
			}
			elsif ($tag eq 'NAME') {
				++$stat{'NAME'}{$arg};
				exists($tags{'AUTHOR'})
					or $fixed{'non-standard order of tags'} = 1;
				if ($arg !~ /^"[ !#-_a-z]{0,120}"$/s) {
					$fatal{'invalid argument of NAME'} = 1;
				}
				elsif ($arg =~ /^"\?*"$/s) {
					$fixed{'blank NAME argument'} = 1;
					$arg = '"<?>"';
				}
			}
			elsif ($tag eq 'DATE') {
				++$stat{'DATE'}{$arg};
				exists($tags{'NAME'})
					or $fixed{'non-standard order of tags'} = 1;
				if ($arg !~ /^"[ !#-_a-z]{0,120}"$/s) {
					$fatal{'invalid argument of DATE'} = 1;
				}
				elsif ($arg =~ /^"\?*"$/s) {
					$fixed{'blank DATE argument'} = 1;
					$arg = '"<?>"';
				}
			}
			elsif ($tag eq 'SONGS') {
				++$stat{'SONGS'}{$arg};
				exists($tags{'DATE'})
					or $fixed{'non-standard order of tags'} = 1;
				if ($arg !~ /^[1-9]\d{0,2}$/s || $arg > 255) {
					$fatal{'invalid argument of SONGS'} = 1;
				}
				elsif ($arg eq '1') {
					$fixed{'SONGS 1 is superfluous'} = 1;
					next;
				}
			}
			elsif ($tag eq 'DEFSONG') {
				exists($tags{'SONGS'})
					or $fixed{'non-standard order of tags'} = 1;
				if ($arg eq '0') {
					$fixed{'DEFSONG 0 is superfluous'} = 1;
					next;
				}
				if ($arg !~ /^[1-9]\d{0,2}$/s) {
					$fatal{'invalid argument of DEFSONG'} = 1;
				}
			}
			elsif ($tag eq 'TYPE') {
				exists($tags{'DATE'})
					or $fixed{'non-standard order of tags'} = 1;
				if ($arg !~ /^[BCDS]$/s) {
					$fatal{'invalid argument of TYPE'} = 1;
				}
			}
			elsif ($tag eq 'FASTPLAY') {
				++$stat{'FASTPLAY'}{$arg};
				exists($tags{'DATE'})
					or $fixed{'non-standard order of tags'} = 1;
				if ($arg !~ /^[1-9]\d{0,2}$/s || $arg > 311) {
					$fatal{'invalid argument of FASTPLAY'} = 1;
				}
			}
			elsif ($tag eq 'STEREO') {
				++$stereo_files;
				exists($tags{'DATE'})
					or $fixed{'non-standard order of tags'} = 1;
				$fatal{'unexpected argument of STEREO'} = 1 if $arg ne '';
			}
			elsif ($tag eq 'INIT' || $tag eq 'PLAYER' || $tag eq 'MUSIC' || $tag eq 'COVOX') {
				exists($tags{'TYPE'})
					or $fixed{'non-standard order of tags'} = 1;
				if ($arg !~ /^[0-9A-Fa-f]{1,4}$/s) {
					$fatal{"invalid argument of $tag"} = 1;
				}
				elsif (length($arg) != 4) {
					$arg = sprintf('%04X', hex($arg));
					$fixed{'hexadecimal values should be 4-digit'} = 1;
				}
				elsif ($arg =~ y/a-f/A-F/) {
					$fixed{'hexadecimal values should be uppercase'} = 1;
				}
			}
			elsif ($tag eq 'TIME') {
				if ($arg !~ /^\d?\d:\d\d(?:\.\d\d\d?)?(?: LOOP)?$/s) {
					$fatal{'invalid argument of TIME'} = 1;
				}
				push @times, $arg;
				next;
			}
			else {
				$fatal{"unknown tag: $tag"} = 1;
				next;
			}
			$tags{$tag} = $arg;
		}
		$fatal{'invalid argument of DEFSONG'} = 1
			if exists($tags{'SONGS'}) && $tags{'DEFSONG'} >= $tags{'SONGS'};
		$fatal{'missing PLAYER tag'} = 1 unless exists($tags{'PLAYER'});
		if (exists($tags{'TYPE'})) {
			$fatal{'INIT is meaningless with TYPE C'} = 1
				if $tags{'TYPE'} eq 'C' && exists($tags{'INIT'});
			$fatal{'MUSIC is meaningful only with TYPE C'} = 1
				if $tags{'TYPE'} ne 'C' && exists($tags{'MUSIC'});
		}
		else {
			$fatal{'missing TYPE tag'} = 1;
		}
		++$types{$tags{'TYPE'}}{exists($tags{'STEREO'})?'stereo':'mono'};
		my $i = 0;
		for (;;) {
			my $bh = substr($bin, $i, 5);
			if (length($bh) < 5) {
				if ($bh ne '') {
					substr($bin, $i) = '';
					$fixed{'garbage bytes at the end'} = 1;
				}
				last;
			}
			my ($start, $end) = unpack('vv', $bh);
			if ($start == 0xffff) {
				$fatal{'duplicate FFFF in the binary part'} = 1;
				last;
			}
			if ($end < $start) {
				$fatal{'block end address less than start address'} = 1;
				last;
			}
			my $ni = $i + 5 + $end - $start;
			if ($ni > length($bin)) {
				substr($bin, $i + 2, 2) =
					pack('v', $start + length($bin) - $i - 5);
				$fixed{'truncated binary block'} = 1;
				last;
			}
			$i = $ni;
			if (substr($bin, $i, 2) eq "\xFF\xFF") {
				substr($bin, $i, 2) = '';
				$fixed{'FFFF inside binary part'} = 1;
			}
		}
		if (%fixed || ($fix && $time && !@times) || $overwrite_time) {
			if (%fatal) {
				push @notfixed_messages,
					"$fullpath (" . join('; ', sort(keys(%fixed))) . ")\n";
			}
			else {
				if ($fix) {
					if (($time && !@times) || $overwrite_time) {
						my $times = `$asapscan -t $filename`;
						if (!$times) {
							$fatal{'error running asapscan'} = 1;
						}
						elsif ($times =~ /^(?:TIME \d?\d:\d\d(?:\.\d\d\d?)?(?: LOOP)?\n)+$/s) {
							my @new_times = $times =~ /\d?\d:\d\d(?:\.\d\d\d?)?(?: LOOP)?/gs;
							if (!@times) {
								@times = @new_times;
								$fixed{'added TIME tags'} = 1;
							}
							elsif ("@new_times" ne "@times") {
								@times = @new_times;
								$fixed{'changed TIME tags'} = 1;
							}
						}
						else {
							$fatal{'cannot detect TIME'} = 1;
						}
					}
					if (%fixed) {
						open F, '>', $filename and binmode F
							or die "$filename: $!\n";
						print F "SAP\x0D\x0A";
						for ('AUTHOR', 'NAME', 'DATE') {
							if (exists($tags{$_})) {
								print F "$_ $tags{$_}\x0D\x0A";
							}
							else {
								$fixed{"missing $_ tag"} = 1;
								print F "$_ \"<?>\"\x0D\x0A";
							}
						}
						print F "SONGS $tags{'SONGS'}\x0D\x0A"
							if exists($tags{'SONGS'});
						print F "DEFSONG $tags{'DEFSONG'}\x0D\x0A"
							if exists($tags{'DEFSONG'});
						print F "STEREO\x0D\x0A" if exists($tags{'STEREO'});
						for ('TYPE', 'FASTPLAY', 'INIT', 'MUSIC', 'PLAYER', 'COVOX') {
							print F "$_ $tags{$_}\x0D\x0A" if exists($tags{$_});
						}
						for (@times) {
							print F "TIME $_\x0D\x0A";
						}
						print F "\xFF\xFF", $bin;
						close F or die "$filename: $!\n";
					}
				}
				if (%fixed) {
					push @fixed_messages,
						"$fullpath (" . join('; ', sort(keys(%fixed))) . ")\n";
				}
			}
		}
		if (@times) {
			++$time_files;
			for (@times) {
				my ($minutes, $seconds, $hundredths, $millis) = /(\d?\d):(\d\d)(?:\.(\d\d)(\d?))?/;
				$total_millis += 60_000 * $minutes + 1000 * $seconds + $hundredths * 10 + $millis;
			}
		}
		elsif ($stat && $time) {
			push @no_time_files, $fullpath
		}
	}
	if ($features) {
		my @features = `$asapscan -f -u $filename`;
		chomp(@features);
		if ($?) {
			$fatal{'error running asapscan'} = 1;
		}
		push @{$features{$_}}, $fullpath for @features;
	}
	if (%fatal) {
		push @fatal_messages,
			"$fullpath (" . join('; ', sort(keys(%fatal))) . ")\n";
	}
}

sub wanted() {
	process($_, $File::Find::name) if /\.sap$/is;
}

Getopt::Long::Configure('bundling');
GetOptions(
	'check|c' => \$check,
	'fix|f' => \$fix,
	'statistics|s' => \$stat,
	'progress|p' => \$progress,
	'time|t' => \$time,
	'overwrite-time|T' => \$overwrite_time,
	'features|u' => \$features,
	'help|h' => \$help,
	'version|v' => \$version,
) or pod2usage(2);

pod2usage({ -verbose => 1, -exitval => 0 }) if $help;
if ($version) {
	print "chksap $VERSION\n";
	exit 0;
}

pod2usage(2) if $check + $fix + $stat != 1 || $time > $fix +$stat || $overwrite_time > $fix || @ARGV == 0;

find(\&wanted, @ARGV);

print "Files processed:              $total_files\n";
print "Not recognized as valid SAP:  ",
	$total_files - $sap_files, "\n" if $sap_files != $total_files;
if ($check) {
	print "Bad files that can be fixed automatically:    ",
		scalar(@fixed_messages), "\n";
	print "Bad files that cannot be fixed automatically: ",
		scalar(@fatal_messages), "\n";
	print "\nPossible automatic fixes:\n",
		@fixed_messages if @fixed_messages;
	print "\nAutomatic fix not possible:\n",
		@fatal_messages if @fatal_messages;
	print "\nCan fix automatically after correcting previous errors:\n",
		@notfixed_messages if @notfixed_messages;
}
elsif ($fix) {
	print "Bad files fixed automatically:                ",
		scalar(@fixed_messages), "\n";
	print "Bad files that cannot be fixed automatically: ",
		scalar(@fatal_messages), "\n";
	print "\nFixed:\n",
		@fixed_messages if @fixed_messages;
	print "\nCannot fix automatically:\n",
		@fatal_messages if @fatal_messages;
	print "\nCan fix after correcting previous errors:\n",
		@notfixed_messages if @notfixed_messages;
}
elsif ($stat) {
	print "Total file size:              $total_length bytes\n";
	printf "Average file size:            %.0f bytes\n", $total_length / $sap_files if $sap_files;
	print "Smallest SAP file:            $min_filename ($min_length bytes)\n" if $min_filename;
	print "Biggest SAP file:             $max_filename ($max_length bytes)\n" if $max_filename;
	my $ref = $stat{'SONGS'};
	if ($ref) {
		print "\nFiles with subsongs:\n";
		my ($files_with_subsongs, $extra_subsongs);
		for (sort { $a <=> $b } keys(%$ref)) {
			next if $_ == 1;
			my $files = $ref->{$_};
			printf "%3s subsongs:%3d file%s\n", $_, $files, $files != 1 ? 's' : '';
			$files_with_subsongs += $files;
			$extra_subsongs += $files * ($_ - 1);
		}
		print "Total: $extra_subsongs extra subsongs in $files_with_subsongs files\n";
	}
	printf "\nFiles tagged with TIME:       $time_files (%d hours %d minutes %d seconds)\n",
		int($total_millis / 3600_000), int($total_millis / 60_000 % 60), $total_millis / 1000 % 60;
	print "\nStereo SAP files:             $stereo_files\n";
	for (sort keys %types) {
		printf "Type %s:   mono:%4d   stereo:%4d   total:%4d\n", $_,
			$types{$_}{'mono'}, $types{$_}{'stereo'}, $types{$_}{'mono'} + $types{$_}{'stereo'};
	}
	for ('FASTPLAY', 'AUTHOR', 'DATE', 'NAME') {
		$ref = $stat{$_};
		if ($ref) {
			print "\n$_ values:\n";
			for (sort keys(%$ref)) {
				printf "%4dx: %s\n", $ref->{$_}, $_;
			}
		}
	}
	if (@no_time_files) {
		print "\nFiles with no TIME tags:\n";
		print "$_\n" for sort @no_time_files;
	}
	if ($features) {
		for (sort keys %features) {
			print "\nFiles which use $_:\n";
			print "$_\n" for sort @{$features{$_}};
		}
	}
}
