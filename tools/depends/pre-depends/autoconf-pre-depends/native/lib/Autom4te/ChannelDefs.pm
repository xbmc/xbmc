# Copyright (C) 2002, 2003, 2006, 2008, 2009, 2010 Free Software
# Foundation, Inc.

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

package Autom4te::ChannelDefs;

use Autom4te::Channels;

=head1 NAME

Autom4te::ChannelDefs - channel definitions for Automake and helper functions

=head1 SYNOPSIS

  use Autom4te::ChannelDefs;

  print Autom4te::ChannelDefs::usage (), "\n";
  prog_error ($MESSAGE, [%OPTIONS]);
  error ($WHERE, $MESSAGE, [%OPTIONS]);
  error ($MESSAGE);
  fatal ($WHERE, $MESSAGE, [%OPTIONS]);
  fatal ($MESSAGE);
  verb ($MESSAGE, [%OPTIONS]);
  switch_warning ($CATEGORY);
  parse_WARNINGS ();
  parse_warnings ($OPTION, @ARGUMENT);
  Autom4te::ChannelDefs::set_strictness ($STRICTNESS_NAME);

=head1 DESCRIPTION

This package defines channels that can be used in Automake to
output diagnostics and other messages (via C<msg()>).  It also defines
some helper function to enable or disable these channels, and some
shorthand function to output on specific channels.

=cut

use 5.005;
use strict;
use Exporter;

use vars qw (@ISA @EXPORT);

@ISA = qw (Exporter);
@EXPORT = qw (&prog_error &error &fatal &verb
	      &switch_warning &parse_WARNINGS &parse_warnings);

=head2 CHANNELS

The following channels can be used as the first argument of
C<Autom4te::Channel::msg>.  For some of them we list a shorthand
function that makes the code more readable.

=over 4

=item C<fatal>

Fatal errors.  Use C<&fatal> to send messages over this channel.

=item C<error>

Common errors.  Use C<&error> to send messages over this channel.

=item C<error-gnu>

Errors related to GNU Standards.

=item C<error-gnu/warn>

Errors related to GNU Standards that should be warnings in "foreign" mode.

=item C<error-gnits>

Errors related to GNITS Standards (silent by default).

=item C<automake>

Internal errors.  Use C<&prog_error> to send messages over this channel.

=item C<cross>

Constructs compromising the cross-compilation of the package.

=item C<gnu>

Warnings related to GNU Coding Standards.

=item C<obsolete>

Warnings about obsolete features (silent by default).

=item C<override>

Warnings about user redefinitions of Automake rules or
variables (silent by default).

=item C<portability>

Warnings about non-portable constructs.

=item C<syntax>

Warnings about weird syntax, unused variables, typos ...

=item C<unsupported>

Warnings about unsupported (or mis-supported) features.

=item C<verb>

Messages output in C<--verbose> mode.  Use C<&verb> to send such messages.

=item C<note>

Informative messages.

=back

=cut

# Initialize our list of error/warning channels.
# Do not forget to update &usage and the manual
# if you add or change a warning channel.

register_channel 'fatal', type => 'fatal', ordered => 0;
register_channel 'error', type => 'error';
register_channel 'error-gnu', type => 'error';
register_channel 'error-gnu/warn', type => 'error';
register_channel 'error-gnits', type => 'error', silent => 1;
register_channel 'automake', type => 'fatal', backtrace => 1,
  header => ("####################\n" .
	     "## Internal Error ##\n" .
	     "####################\n"),
  footer => "\nPlease contact <bug-automake\@gnu.org>.",
  ordered => 0;

register_channel 'cross', type => 'warning', silent => 1;
register_channel 'gnu', type => 'warning';
register_channel 'obsolete', type => 'warning', silent => 1;
register_channel 'override', type => 'warning', silent => 1;
register_channel 'portability', type => 'warning', silent => 1;
register_channel 'syntax', type => 'warning';
register_channel 'unsupported', type => 'warning';

register_channel 'verb', type => 'debug', silent => 1, ordered => 0;
register_channel 'note', type => 'debug', silent => 0;

=head2 FUNCTIONS

=over 4

=item C<usage ()>

Return the warning category descriptions.

=cut

sub usage ()
{
  return "Warning categories include:
  `cross'         cross compilation issues
  `gnu'           GNU coding standards (default in gnu and gnits modes)
  `obsolete'      obsolete features or constructions
  `override'      user redefinitions of Automake rules or variables
  `portability'   portability issues (default in gnu and gnits modes)
  `syntax'        dubious syntactic constructs (default)
  `unsupported'   unsupported or incomplete features (default)
  `all'           all the warnings
  `no-CATEGORY'   turn off warnings in CATEGORY
  `none'          turn off all the warnings
  `error'         treat warnings as errors";
}

=item C<prog_error ($MESSAGE, [%OPTIONS])>

Signal a programming error (on channel C<automake>),
display C<$MESSAGE>, and exit 1.

=cut

sub prog_error ($;%)
{
  my ($msg, %opts) = @_;
  msg 'automake', '', $msg, %opts;
}

=item C<error ($WHERE, $MESSAGE, [%OPTIONS])>

=item C<error ($MESSAGE)>

Uncategorized errors.

=cut

sub error ($;$%)
{
  my ($where, $msg, %opts) = @_;
  msg ('error', $where, $msg, %opts);
}

=item C<fatal ($WHERE, $MESSAGE, [%OPTIONS])>

=item C<fatal ($MESSAGE)>

Fatal errors.

=cut

sub fatal ($;$%)
{
  my ($where, $msg, %opts) = @_;
  msg ('fatal', $where, $msg, %opts);
}

=item C<verb ($MESSAGE, [%OPTIONS])>

C<--verbose> messages.

=cut

sub verb ($;%)
{
  my ($msg, %opts) = @_;
  msg 'verb', '', $msg, %opts;
}

=item C<switch_warning ($CATEGORY)>

If C<$CATEGORY> is C<mumble>, turn on channel C<mumble>.
If it is C<no-mumble>, turn C<mumble> off.
Else handle C<all> and C<none> for completeness.

=cut

sub switch_warning ($)
{
  my ($cat) = @_;
  my $has_no = 0;

  if ($cat =~ /^no-(.*)$/)
    {
      $cat = $1;
      $has_no = 1;
    }

  if ($cat eq 'all')
    {
      setup_channel_type 'warning', silent => $has_no;
    }
  elsif ($cat eq 'none')
    {
      setup_channel_type 'warning', silent => ! $has_no;
    }
  elsif ($cat eq 'error')
    {
      $warnings_are_errors = ! $has_no;
      # Set exit code if Perl warns about something
      # (like uninitialized variables).
      $SIG{"__WARN__"} =
	$has_no ? 'DEFAULT' : sub { print STDERR @_; $exit_code = 1; };
    }
  elsif (channel_type ($cat) eq 'warning')
    {
      setup_channel $cat, silent => $has_no;
    }
  else
    {
      return 1;
    }
  return 0;
}

=item C<parse_WARNINGS ()>

Parse the WARNINGS environment variable.

=cut

sub parse_WARNINGS ()
{
  if (exists $ENV{'WARNINGS'})
    {
      # Ignore unknown categories.  This is required because WARNINGS
      # should be honored by many tools.
      switch_warning $_ foreach (split (',', $ENV{'WARNINGS'}));
    }
}

=item C<parse_warnings ($OPTION, @ARGUMENT)>

Parse the argument of C<--warning=CATEGORY> or C<-WCATEGORY>.

C<$OPTIONS> is C<"--warning"> or C<"-W">, C<@ARGUMENT> is a list of
C<CATEGORY>.

This can be used as an argument to C<Getopt>.

=cut

sub parse_warnings ($@)
{
  my ($opt, @categories) = @_;

  foreach my $cat (map { split ',' } @categories)
    {
      msg 'unsupported', "unknown warning category `$cat'"
	if switch_warning $cat;
    }
}

=item C<set_strictness ($STRICTNESS_NAME)>

Configure channels for strictness C<$STRICTNESS_NAME>.

=cut

sub set_strictness ($)
{
  my ($name) = @_;

  if ($name eq 'gnu')
    {
      setup_channel 'error-gnu', silent => 0;
      setup_channel 'error-gnu/warn', silent => 0, type => 'error';
      setup_channel 'error-gnits', silent => 1;
      setup_channel 'portability', silent => 0;
      setup_channel 'gnu', silent => 0;
    }
  elsif ($name eq 'gnits')
    {
      setup_channel 'error-gnu', silent => 0;
      setup_channel 'error-gnu/warn', silent => 0, type => 'error';
      setup_channel 'error-gnits', silent => 0;
      setup_channel 'portability', silent => 0;
      setup_channel 'gnu', silent => 0;
    }
  elsif ($name eq 'foreign')
    {
      setup_channel 'error-gnu', silent => 1;
      setup_channel 'error-gnu/warn', silent => 0, type => 'warning';
      setup_channel 'error-gnits', silent => 1;
      setup_channel 'portability', silent => 1;
      setup_channel 'gnu', silent => 1;
    }
  else
    {
      prog_error "level `$name' not recognized\n";
    }
}

=back

=head1 SEE ALSO

L<Autom4te::Channels>

=head1 HISTORY

Written by Alexandre Duret-Lutz E<lt>F<adl@gnu.org>E<gt>.

=cut

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
