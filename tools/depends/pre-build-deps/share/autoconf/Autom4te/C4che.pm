# autoconf -- create `configure' using m4 macros
# Copyright (C) 2003, 2006, 2009, 2010 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

package Autom4te::C4che;

=head1 NAME

Autom4te::C4che - a single m4 run request

=head1 SYNOPSIS

  use Autom4te::C4che;

=head1 DESCRIPTION

This Perl module handles the cache of M4 runs used by autom4te.

=cut

use Data::Dumper;
use Autom4te::Request;
use Carp;
use strict;

=over 4

=item @request

List of requests.

We cannot declare it "my" as the loading, performed via "do", would
refer to another scope, and @request would not be updated.  It used to
work with "my" vars, and I do not know whether the current behavior
(5.6) is wanted or not.

=cut

use vars qw(@request);

=item C<$req = Autom4te::C4che-E<gt>retrieve (%attr)>

Find a request with the same path and input.

=cut

sub retrieve($%)
{
  my ($self, %attr) = @_;

  foreach (@request)
    {
      # Same path.
      next
	if join ("\n", @{$_->path}) ne join ("\n", @{$attr{path}});

      # Same inputs.
      next
	if join ("\n", @{$_->input}) ne join ("\n", @{$attr{input}});

      # Found it.
      return $_;
    }

  return undef;
}

=item C<$req = Autom4te::C4che-E<gt>register (%attr)>

Create and register a request for these path and input.

=cut

# $REQUEST-OBJ
# register ($SELF, %ATTR)
# -----------------------
# NEW should not be called directly.
# Private.
sub register ($%)
{
  my ($self, %attr) = @_;

  # path and input are the only ID for a request object.
  my $obj = new Autom4te::Request ('path'  => $attr{path},
				   'input' => $attr{input});
  push @request, $obj;

  # Assign an id for cache file.
  $obj->id ("$#request");

  return $obj;
}


=item C<$req = Autom4te::C4che-E<gt>request (%request)>

Get (retrieve or create) a request for the path C<$request{path}> and
the input C<$request{input}>.

=cut

# $REQUEST-OBJ
# request($SELF, %REQUEST)
# ------------------------
sub request ($%)
{
  my ($self, %request) = @_;

  my $req =
    Autom4te::C4che->retrieve (%request)
    || Autom4te::C4che->register (%request);

  # If there are new traces to produce, then we are not valid.
  foreach (@{$request{'macro'}})
    {
      if (! exists ${$req->macro}{$_})
	{
	  ${$req->macro}{$_} = 1;
	  $req->valid (0);
	}
    }

  # It would be great to have $REQ check that it is up to date wrt
  # its dependencies, but that requires getting traces (to fetch the
  # included files), which is out of the scope of Request (currently?).

  return $req;
}


=item C<$string = Autom4te::C4che-E<gt>marshall ()>

Serialize all the current requests.

=cut


# marshall($SELF)
# ---------------
sub marshall ($)
{
  my ($caller) = @_;
  my $res = '';

  my $marshall = Data::Dumper->new ([\@request], [qw (*request)]);
  $marshall->Indent(2)->Terse(0);
  $res = $marshall->Dump . "\n";

  return $res;
}


=item C<Autom4te::C4che-E<gt>save ($file)>

Save the cache in the C<$file> file object.

=cut

# SAVE ($FILE)
# ------------
sub save ($$)
{
  my ($self, $file) = @_;

  confess "cannot save a single request\n"
    if ref ($self);

  $file->seek (0, 0);
  $file->truncate (0);
  print $file
    "# This file was generated.\n",
    "# It contains the lists of macros which have been traced.\n",
    "# It can be safely removed.\n",
    "\n",
    $self->marshall;
}


=item C<Autom4te::C4che-E<gt>load ($file)>

Load the cache from the C<$file> file object.

=cut

# LOAD ($FILE)
# ------------
sub load ($$)
{
  my ($self, $file) = @_;
  my $fname = $file->name;

  confess "cannot load a single request\n"
    if ref ($self);

  my $contents = join "", $file->getlines;

  eval $contents;

  confess "cannot eval $fname: $@\n" if $@;
}


=head1 SEE ALSO

L<Autom4te::Request>

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
