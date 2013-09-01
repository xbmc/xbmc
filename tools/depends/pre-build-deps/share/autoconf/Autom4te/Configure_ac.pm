# Copyright (C) 2003, 2005, 2006, 2009, 2010 Free Software Foundation,
# Inc.

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

package Autom4te::Configure_ac;

use strict;
use Exporter;
use Autom4te::Channels;
use Autom4te::ChannelDefs;

use vars qw (@ISA @EXPORT);

@ISA = qw (Exporter);
@EXPORT = qw (&find_configure_ac &require_configure_ac);

=head1 NAME

Autom4te::Configure_ac - Locate configure.ac or configure.in.

=head1 SYNOPSIS

  use Autom4te::Configure_ac;

  # Try to locate configure.in or configure.ac in the current
  # directory.  It may be absent.  Complain if both files exist.
  my $file_name = find_configure_ac;

  # Likewise, but bomb out if the file does not exist.
  my $file_name = require_configure_ac;

  # Likewise, but in $dir.
  my $file_name = find_configure_ac ($dir);
  my $file_name = require_configure_ac ($dir);

=over 4

=back

=head2 Functions

=over 4

=item C<$configure_ac = find_configure_ac ([$directory])>

Find a F<configure.ac> or F<configure.in> file in C<$directory>,
defaulting to the current directory.  Complain if both files are present.
Return the name of the file found, or the former if neither is present.

=cut

sub find_configure_ac (;@)
{
  my ($directory) = @_;
  $directory ||= '.';
  my $configure_ac =
    File::Spec->canonpath (File::Spec->catfile ($directory, 'configure.ac'));
  my $configure_in =
    File::Spec->canonpath (File::Spec->catfile ($directory, 'configure.in'));

  if (-f $configure_ac)
    {
      if (-f $configure_in)
	{
	  msg ('unsupported',
	       "`$configure_ac' and `$configure_in' both present.\n"
	       . "proceeding with `$configure_ac'");
	}
      return $configure_ac
    }
  elsif (-f $configure_in)
    {
      return $configure_in;
    }
  return $configure_ac;
}


=item C<$configure_ac = require_configure_ac ([$directory])>

Like C<find_configure_ac>, but fail if neither is present.

=cut

sub require_configure_ac (;$)
{
  my $res = find_configure_ac (@_);
  fatal "`configure.ac' or `configure.in' is required"
    unless -f $res;
  return $res
}

1;

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
