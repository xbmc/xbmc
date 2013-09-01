# autoconf -- create `configure' using m4 macros
# Copyright (C) 2001, 2002, 2003, 2009, 2010 Free Software Foundation,
# Inc.

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

package Autom4te::Request;

=head1 NAME

Autom4te::Request - a single m4 run request

=head1 SYNOPSIS

  use Autom4te::Request;

=head1 DESCRIPTION

This perl module provides various general purpose support functions
used in several executables of the Autoconf and Automake packages.

=cut

use strict;
use Autom4te::Struct;
use Carp;
use Data::Dumper;

struct
  (
   # The key of the cache files.
   'id' => "\$",
   # True iff %MACRO contains all the macros we want to trace.
   'valid' => "\$",
   # The include path.
   'path' => '@',
   # The set of input files.
   'input' => '@',
   # The set of macros currently traced.
   'macro' => '%',
  );


# Serialize a request or all the current requests.
sub marshall($)
{
  my ($caller) = @_;
  my $res = '';

  # CALLER is an object: instance method.
  my $marshall = Data::Dumper->new ([$caller]);
  $marshall->Indent(2)->Terse(0);
  $res = $marshall->Dump . "\n";

  return $res;
}


# includes_p ($SELF, @MACRO)
# --------------------------
# Does this request covers all the @MACRO.
sub includes_p
{
  my ($self, @macro) = @_;

  foreach (@macro)
    {
      return 0
	if ! exists ${$self->macro}{$_};
    }
  return 1;
}


=head1 SEE ALSO

L<Autom4te::C4che>

=head1 HISTORY

Written by Akim Demaille E<lt>F<akim@freefriends.org>E<gt>.

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
