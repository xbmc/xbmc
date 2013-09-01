# Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2010 Free Software
# Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

###############################################################
# The main copy of this file is in Automake's CVS repository. #
# Updates should be sent to automake-patches@gnu.org.         #
###############################################################

package Autom4te::FileUtils;

=head1 NAME

Autom4te::FileUtils - handling files

=head1 SYNOPSIS

  use Autom4te::FileUtils

=head1 DESCRIPTION

This perl module provides various general purpose file handling functions.

=cut

use strict;
use Exporter;
use File::stat;
use IO::File;
use Autom4te::Channels;
use Autom4te::ChannelDefs;

use vars qw (@ISA @EXPORT);

@ISA = qw (Exporter);
@EXPORT = qw (&open_quote &contents
	      &find_file &mtime
	      &update_file &up_to_date_p
	      &xsystem &xsystem_hint &xqx
	      &dir_has_case_matching_file &reset_dir_cache
	      &set_dir_cache_file);


=item C<open_quote ($file_name)>

Quote C<$file_name> for open.

=cut

# $FILE_NAME
# open_quote ($FILE_NAME)
# -----------------------
# If the string $S is a well-behaved file name, simply return it.
# If it starts with white space, prepend `./', if it ends with
# white space, add `\0'.  Return the new string.
sub open_quote($)
{
  my ($s) = @_;
  if ($s =~ m/^\s/)
    {
      $s = "./$s";
    }
  if ($s =~ m/\s$/)
    {
      $s = "$s\0";
    }
  return $s;
}

=item C<find_file ($file_name, @include)>

Return the first path for a C<$file_name> in the C<include>s.

We match exactly the behavior of GNU M4: first look in the current
directory (which includes the case of absolute file names), and then,
if the file name is not absolute, look in C<@include>.

If the file is flagged as optional (ends with C<?>), then return undef
if absent, otherwise exit with error.

=cut

# $FILE_NAME
# find_file ($FILE_NAME, @INCLUDE)
# --------------------------------
sub find_file ($@)
{
  use File::Spec;

  my ($file_name, @include) = @_;
  my $optional = 0;

  $optional = 1
    if $file_name =~ s/\?$//;

  return File::Spec->canonpath ($file_name)
    if -e $file_name;

  if (!File::Spec->file_name_is_absolute ($file_name))
    {
      foreach my $path (@include)
	{
	  return File::Spec->canonpath (File::Spec->catfile ($path, $file_name))
	    if -e File::Spec->catfile ($path, $file_name)
	}
    }

  fatal "$file_name: no such file or directory"
    unless $optional;
  return undef;
}

=item C<mtime ($file)>

Return the mtime of C<$file>.  Missing files, or C<-> standing for
C<STDIN> or C<STDOUT> are ``obsolete'', i.e., as old as possible.

=cut

# $MTIME
# MTIME ($FILE)
# -------------
sub mtime ($)
{
  my ($file) = @_;

  return 0
    if $file eq '-' || ! -f $file;

  my $stat = stat ($file)
    or fatal "cannot stat $file: $!";

  return $stat->mtime;
}


=item C<update_file ($from, $to, [$force])>

Rename C<$from> as C<$to>, preserving C<$to> timestamp if it has not
changed, unless C<$force> is true (defaults to false).  Recognize
C<$to> = C<-> standing for C<STDIN>.  C<$from> is always
removed/renamed.

=cut

# &update_file ($FROM, $TO; $FORCE)
# ---------------------------------
sub update_file ($$;$)
{
  my ($from, $to, $force) = @_;
  $force = 0
    unless defined $force;
  my $SIMPLE_BACKUP_SUFFIX = $ENV{'SIMPLE_BACKUP_SUFFIX'} || '~';
  use File::Compare;
  use File::Copy;

  if ($to eq '-')
    {
      my $in = new IO::File ("< " . open_quote ($from));
      my $out = new IO::File (">-");
      while ($_ = $in->getline)
	{
	  print $out $_;
	}
      $in->close;
      unlink ($from) || fatal "cannot remove $from: $!";
      return;
    }

  if (!$force && -f "$to" && compare ("$from", "$to") == 0)
    {
      # File didn't change, so don't update its mod time.
      msg 'note', "`$to' is unchanged";
      unlink ($from)
        or fatal "cannot remove $from: $!";
      return
    }

  if (-f "$to")
    {
      # Back up and install the new one.
      move ("$to",  "$to$SIMPLE_BACKUP_SUFFIX")
	or fatal "cannot backup $to: $!";
      move ("$from", "$to")
	or fatal "cannot rename $from as $to: $!";
      msg 'note', "`$to' is updated";
    }
  else
    {
      move ("$from", "$to")
	or fatal "cannot rename $from as $to: $!";
      msg 'note', "`$to' is created";
    }
}


=item C<up_to_date_p ($file, @dep)>

Is C<$file> more recent than C<@dep>?

=cut

# $BOOLEAN
# &up_to_date_p ($FILE, @DEP)
# ---------------------------
sub up_to_date_p ($@)
{
  my ($file, @dep) = @_;
  my $mtime = mtime ($file);

  foreach my $dep (@dep)
    {
      if ($mtime < mtime ($dep))
	{
	  verb "up_to_date ($file): outdated: $dep";
	  return 0;
	}
    }

  verb "up_to_date ($file): up to date";
  return 1;
}


=item C<handle_exec_errors ($command, [$expected_exit_code = 0], [$hint])>

Display an error message for C<$command>, based on the content of
C<$?> and C<$!>.  Be quiet if the command exited normally
with C<$expected_exit_code>.  If C<$hint> is given, display that as well
if the command failed to run at all.

=cut

sub handle_exec_errors ($;$$)
{
  my ($command, $expected, $hint) = @_;
  $expected = 0 unless defined $expected;
  if (defined $hint)
    {
      $hint = "\n" . $hint;
    }
  else
    {
      $hint = '';
    }

  $command = (split (' ', $command))[0];
  if ($!)
    {
      fatal "failed to run $command: $!" . $hint;
    }
  else
    {
      use POSIX qw (WIFEXITED WEXITSTATUS WIFSIGNALED WTERMSIG);

      if (WIFEXITED ($?))
	{
	  my $status = WEXITSTATUS ($?);
	  # Propagate exit codes.
	  fatal ('',
		 "$command failed with exit status: $status",
		 exit_code => $status)
	    unless $status == $expected;
	}
      elsif (WIFSIGNALED ($?))
	{
	  my $signal = WTERMSIG ($?);
	  fatal "$command terminated by signal: $signal";
	}
      else
	{
	  fatal "$command exited abnormally";
	}
    }
}

=item C<xqx ($command)>

Same as C<qx> (but in scalar context), but fails on errors.

=cut

# xqx ($COMMAND)
# --------------
sub xqx ($)
{
  my ($command) = @_;

  verb "running: $command";

  $! = 0;
  my $res = `$command`;
  handle_exec_errors $command
    if $?;

  return $res;
}


=item C<xsystem (@argv)>

Same as C<system>, but fails on errors, and reports the C<@argv>
in verbose mode.

=cut

sub xsystem (@)
{
  my (@command) = @_;

  verb "running: @command";

  $! = 0;
  handle_exec_errors "@command"
    if system @command;
}


=item C<xsystem_hint ($msg, @argv)>

Same as C<xsystem>, but allows to pass a hint that will be displayed
in case the command failed to run at all.

=cut

sub xsystem_hint (@)
{
  my ($hint, @command) = @_;

  verb "running: @command";

  $! = 0;
  handle_exec_errors "@command", 0, $hint
    if system @command;
}


=item C<contents ($file_name)>

Return the contents of C<$file_name>.

=cut

# contents ($FILE_NAME)
# ---------------------
sub contents ($)
{
  my ($file) = @_;
  verb "reading $file";
  local $/;			# Turn on slurp-mode.
  my $f = new Autom4te::XFile "< " . open_quote ($file);
  my $contents = $f->getline;
  $f->close;
  return $contents;
}


=item C<dir_has_case_matching_file ($DIRNAME, $FILE_NAME)>

Return true iff $DIR contains a file name that matches $FILE_NAME case
insensitively.

We need to be cautious on case-insensitive case-preserving file
systems (e.g. Mac OS X's HFS+).  On such systems C<-f 'Foo'> and C<-f
'foO'> answer the same thing.  Hence if a package distributes its own
F<CHANGELOG> file, but has no F<ChangeLog> file, automake would still
try to distribute F<ChangeLog> (because it thinks it exists) in
addition to F<CHANGELOG>, although it is impossible for these two
files to be in the same directory (the two file names designate the
same file).

=cut

use vars '%_directory_cache';
sub dir_has_case_matching_file ($$)
{
  # Note that print File::Spec->case_tolerant returns 0 even on MacOS
  # X (with Perl v5.8.1-RC3 at least), so do not try to shortcut this
  # function using that.

  my ($dirname, $file_name) = @_;
  return 0 unless -f "$dirname/$file_name";

  # The file appears to exist, however it might be a mirage if the
  # system is case insensitive.  Let's browse the directory and check
  # whether the file is really in.  We maintain a cache of directories
  # so Automake doesn't spend all its time reading the same directory
  # again and again.
  if (!exists $_directory_cache{$dirname})
    {
      error "failed to open directory `$dirname'"
	unless opendir (DIR, $dirname);
      $_directory_cache{$dirname} = { map { $_ => 1 } readdir (DIR) };
      closedir (DIR);
    }
  return exists $_directory_cache{$dirname}{$file_name};
}

=item C<reset_dir_cache ($dirname)>

Clear C<dir_has_case_matching_file>'s cache for C<$dirname>.

=cut

sub reset_dir_cache ($)
{
  delete $_directory_cache{$_[0]};
}

=item C<set_dir_cache_file ($dirname, $file_name)>

State that C<$dirname> contains C<$file_name> now.

=cut

sub set_dir_cache_file ($$)
{
  my ($dirname, $file_name) = @_;
  $_directory_cache{$dirname}{$file_name} = 1
    if exists $_directory_cache{$dirname};
}

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
