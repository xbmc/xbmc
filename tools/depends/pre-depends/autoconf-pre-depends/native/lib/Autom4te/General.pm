# autoconf -- create `configure' using m4 macros
# Copyright (C) 2001, 2002, 2003, 2004, 2006, 2007, 2009, 2010 Free
# Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

package Autom4te::General;

=head1 NAME

Autom4te::General - general support functions for Autoconf and Automake

=head1 SYNOPSIS

  use Autom4te::General

=head1 DESCRIPTION

This perl module provides various general purpose support functions
used in several executables of the Autoconf and Automake packages.

=cut

use 5.005_03;
use Exporter;
use Autom4te::ChannelDefs;
use Autom4te::Channels;
use File::Basename;
use File::Path ();
use File::stat;
use IO::File;
use Carp;
use strict;

use vars qw (@ISA @EXPORT);

@ISA = qw (Exporter);

# Variables we define and export.
my @export_vars =
  qw ($debug $force $help $me $tmp $verbose $version);

# Functions we define and export.
my @export_subs =
  qw (&debug
      &getopt &shell_quote &mktmpdir
      &uniq);

# Functions we forward (coming from modules we use).
my @export_forward_subs =
  qw (&basename &dirname &fileparse);

@EXPORT = (@export_vars, @export_subs, @export_forward_subs);


# Variable we share with the main package.  Be sure to have a single
# copy of them: using `my' together with multiple inclusion of this
# package would introduce several copies.

=head2 Global Variables

=over 4

=item C<$debug>

Set this variable to 1 if debug messages should be enabled.  Debug
messages are meant for developpers only, or when tracking down an
incorrect execution.

=cut

use vars qw ($debug);
$debug = 0;

=item C<$force>

Set this variable to 1 to recreate all the files, or to consider all
the output files are obsolete.

=cut

use vars qw ($force);
$force = undef;

=item C<$help>

Set to the help message associated with the option C<--help>.

=cut

use vars qw ($help);
$help = undef;

=item C<$me>

The name of this application, for diagnostic messages.

=cut

use vars qw ($me);
$me = basename ($0);

=item C<$tmp>

The name of the temporary directory created by C<mktmpdir>.  Left
C<undef> otherwise.

=cut

# Our tmp dir.
use vars qw ($tmp);
$tmp = undef;

=item C<$verbose>

Enable verbosity messages.  These messages are meant for ordinary
users, and typically make explicit the steps being performed.

=cut

use vars qw ($verbose);
$verbose = 0;

=item C<$version>

Set to the version message associated to the option C<--version>.

=cut

use vars qw ($version);
$version = undef;

=back

=cut



## ----- ##
## END.  ##
## ----- ##

=head2 Functions

=over 4

=item C<END>

Filter Perl's exit codes, delete any temporary directory (unless
C<$debug>), and exit nonzero whenever closing C<STDOUT> fails.

=cut

# END
# ---
sub END
{
  # $? contains the exit status we will return.
  # It was set using one of the following ways:
  #
  #  1) normal termination
  #     this sets $? = 0
  #  2) calling `exit (n)'
  #     this sets $? = n
  #  3) calling die or friends (croak, confess...):
  #     a) when $! is non-0
  #        this set $? = $!
  #     b) when $! is 0 but $? is not
  #        this sets $? = ($? >> 8)   (i.e., the exit code of the
  #        last program executed)
  #     c) when both $! and $? are 0
  #        this sets $? = 255
  #
  # Cases 1), 2), and 3b) are fine, but we prefer $? = 1 for 3a) and 3c).
  my $status = $?;
  $status = 1 if ($! && $! == $?) || $? == 255;
  # (Note that we cannot safely distinguish calls to `exit (n)'
  # from calls to die when `$! = n'.  It's not big deal because
  # we only call `exit (0)' or `exit (1)'.)

  if (!$debug && defined $tmp && -d $tmp)
    {
      local $SIG{__WARN__} = sub { $status = 1; warn $_[0] };
      File::Path::rmtree $tmp;
    }

  # This is required if the code might send any output to stdout
  # E.g., even --version or --help.  So it's best to do it unconditionally.
  if (! close STDOUT)
    {
      print STDERR "$me: closing standard output: $!\n";
      $? = 1;
      return;
    }

  $? = $status;
}


## ----------- ##
## Functions.  ##
## ----------- ##


=item C<debug (@message)>

If the debug mode is enabled (C<$debug> and C<$verbose>), report the
C<@message> on C<STDERR>, signed with the name of the program.

=cut

# &debug(@MESSAGE)
# ----------------
# Messages displayed only if $DEBUG and $VERBOSE.
sub debug (@)
{
  print STDERR "$me: ", @_, "\n"
    if $verbose && $debug;
}


=item C<getopt (%option)>

Wrapper around C<Getopt::Long>.  In addition to the user C<option>s,
support C<-h>/C<--help>, C<-V>/C<--version>, C<-v>/C<--verbose>,
C<-d>/C<--debug>, C<-f>/C<--force>.  Conform to the GNU Coding
Standards for error messages.  Try to work around a weird behavior
from C<Getopt::Long> to preserve C<-> as an C<@ARGV> instead of
rejecting it as a broken option.

=cut

# getopt (%OPTION)
# ----------------
# Handle the %OPTION, plus all the common options.
# Work around Getopt bugs wrt `-'.
sub getopt (%)
{
  my (%option) = @_;
  use Getopt::Long;

  # F*k.  Getopt seems bogus and dies when given `-' with `bundling'.
  # If fixed some day, use this: '' => sub { push @ARGV, "-" }
  my $stdin = grep /^-$/, @ARGV;
  @ARGV = grep !/^-$/, @ARGV;
  %option = ("h|help"     => sub { print $help; exit 0 },
	     "V|version"  => sub { print $version; exit 0 },

	     "v|verbose"  => sub { ++$verbose },
	     "d|debug"    => sub { ++$debug },
	     'f|force'    => \$force,

	     # User options last, so that they have precedence.
	     %option);
  Getopt::Long::Configure ("bundling", "pass_through");
  GetOptions (%option)
    or exit 1;

  foreach (grep { /^-./ } @ARGV)
    {
      print STDERR "$0: unrecognized option `$_'\n";
      print STDERR "Try `$0 --help' for more information.\n";
      exit (1);
    }

  push @ARGV, '-'
    if $stdin;

  setup_channel 'note', silent => !$verbose;
  setup_channel 'verb', silent => !$verbose;
}


=item C<shell_quote ($file_name)>

Quote C<$file_name> for the shell.

=cut

# $FILE_NAME
# shell_quote ($FILE_NAME)
# ------------------------
# If the string $S is a well-behaved file name, simply return it.
# If it contains white space, quotes, etc., quote it, and return
# the new string.
sub shell_quote($)
{
  my ($s) = @_;
  if ($s =~ m![^\w+/.,-]!)
    {
      # Convert each single quote to '\''
      $s =~ s/\'/\'\\\'\'/g;
      # Then single quote the string.
      $s = "'$s'";
    }
  return $s;
}

=item C<mktmpdir ($signature)>

Create a temporary directory which name is based on C<$signature>.
Store its name in C<$tmp>.  C<END> is in charge of removing it, unless
C<$debug>.

=cut

# mktmpdir ($SIGNATURE)
# ---------------------
sub mktmpdir ($)
{
  my ($signature) = @_;
  my $TMPDIR = $ENV{'TMPDIR'} || '/tmp';
  my $quoted_tmpdir = shell_quote ($TMPDIR);

  # If mktemp supports dirs, use it.
  $tmp = `(umask 077 &&
	   mktemp -d $quoted_tmpdir/"${signature}XXXXXX") 2>/dev/null`;
  chomp $tmp;

  if (!$tmp || ! -d $tmp)
    {
      $tmp = "$TMPDIR/$signature" . int (rand 10000) . ".$$";
      mkdir $tmp, 0700
	or croak "$me: cannot create $tmp: $!\n";
    }

  print STDERR "$me:$$: working in $tmp\n"
    if $debug;
}


=item C<uniq (@list)>

Return C<@list> with no duplicates, keeping only the first
occurrences.

=cut

# @RES
# uniq (@LIST)
# ------------
sub uniq (@)
{
  my @res = ();
  my %seen = ();
  foreach my $item (@_)
    {
      if (! exists $seen{$item})
	{
	  $seen{$item} = 1;
	  push (@res, $item);
	}
    }
  return wantarray ? @res : "@res";
}


=item C<handle_exec_errors ($command)>

Display an error message for C<$command>, based on the content of
C<$?> and C<$!>.

=cut


# handle_exec_errors ($COMMAND)
# -----------------------------
sub handle_exec_errors ($)
{
  my ($command) = @_;

  $command = (split (' ', $command))[0];
  if ($!)
    {
      error "failed to run $command: $!";
    }
  else
    {
      use POSIX qw (WIFEXITED WEXITSTATUS WIFSIGNALED WTERMSIG);

      if (WIFEXITED ($?))
	{
	  my $status = WEXITSTATUS ($?);
	  # WIFEXITED and WEXITSTATUS can alter $!, reset it so that
	  # error() actually propagates the command's exit status, not $!.
	  $! = 0;
	  error "$command failed with exit status: $status";
	}
      elsif (WIFSIGNALED ($?))
	{
	  my $signal = WTERMSIG ($?);
	  # In this case we prefer to exit with status 1.
	  $! = 1;
	  error "$command terminated by signal: $signal";
	}
      else
	{
	  error "$command exited abnormally";
	}
    }
}

=back

=head1 SEE ALSO

L<Autom4te::XFile>

=head1 HISTORY

Written by Alexandre Duret-Lutz E<lt>F<adl@gnu.org>E<gt> and Akim
Demaille E<lt>F<akim@freefriends.org>E<gt>.

=cut



1; # for require

### Setup "GNU" style for perl-mode and cperl-mode.
## Local Variables:
## perl-indent-level: 2
## perl-continued-statement-offset: 2
## perl-continued-brace-offset: 0
## perl-brace-offset: 0
## perl-brace-imaginary-offset: 0
## perl-label-offset: -2
## cperl-indent-level: 2
## cperl-brace-offset: 0
## cperl-continued-brace-offset: 0
## cperl-label-offset: -2
## cperl-extra-newline-before-brace: t
## cperl-merge-trailing-else: nil
## cperl-continued-statement-offset: 2
## End:
