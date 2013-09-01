# Copyright (C) 2001, 2003, 2004, 2006, 2008, 2009, 2010 Free Software
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

# Written by Akim Demaille <akim@freefriends.org>.

###############################################################
# The main copy of this file is in Automake's CVS repository. #
# Updates should be sent to automake-patches@gnu.org.         #
###############################################################

package Autom4te::XFile;

=head1 NAME

Autom4te::XFile - supply object methods for filehandles with error handling

=head1 SYNOPSIS

    use Autom4te::XFile;

    $fh = new Autom4te::XFile;
    $fh->open ("< file");
    # No need to check $FH: we died if open failed.
    print <$fh>;
    $fh->close;
    # No need to check the return value of close: we died if it failed.

    $fh = new Autom4te::XFile "> file";
    # No need to check $FH: we died if new failed.
    print $fh "bar\n";
    $fh->close;

    $fh = new Autom4te::XFile "file", "r";
    # No need to check $FH: we died if new failed.
    defined $fh
    print <$fh>;
    undef $fh;   # automatically closes the file and checks for errors.

    $fh = new Autom4te::XFile "file", O_WRONLY | O_APPEND;
    # No need to check $FH: we died if new failed.
    print $fh "corge\n";

    $pos = $fh->getpos;
    $fh->setpos ($pos);

    undef $fh;   # automatically closes the file and checks for errors.

    autoflush STDOUT 1;

=head1 DESCRIPTION

C<Autom4te::XFile> inherits from C<IO::File>.  It provides the method
C<name> returning the file name.  It provides dying versions of the
methods C<close>, C<lock> (corresponding to C<flock>), C<new>,
C<open>, C<seek>, and C<truncate>.  It also overrides the C<getline>
and C<getlines> methods to translate C<\r\n> to C<\n>.

=cut

require 5.000;
use strict;
use vars qw($VERSION @EXPORT @EXPORT_OK $AUTOLOAD @ISA);
use Carp;
use Errno;
use IO::File;
use File::Basename;
use Autom4te::ChannelDefs;
use Autom4te::Channels qw(msg);
use Autom4te::FileUtils;

require Exporter;
require DynaLoader;

@ISA = qw(IO::File Exporter DynaLoader);

$VERSION = "1.2";

@EXPORT = @IO::File::EXPORT;

eval {
  # Make all Fcntl O_XXX and LOCK_XXX constants available for importing
  require Fcntl;
  my @O = grep /^(LOCK|O)_/, @Fcntl::EXPORT, @Fcntl::EXPORT_OK;
  Fcntl->import (@O);  # first we import what we want to export
  push (@EXPORT, @O);
};

=head2 Methods

=over

=item C<$fh = new Autom4te::XFile ([$expr, ...]>

Constructor a new XFile object.  Additional arguments
are passed to C<open>, if any.

=cut

sub new
{
  my $type = shift;
  my $class = ref $type || $type || "Autom4te::XFile";
  my $fh = $class->SUPER::new ();
  if (@_)
    {
      $fh->open (@_);
    }
  $fh;
}

=item C<$fh-E<gt>open ([$file, ...])>

Open a file, passing C<$file> and further arguments to C<IO::File::open>.
Die if opening fails.  Store the name of the file.  Use binmode for writing.

=cut

sub open
{
  my $fh = shift;
  my ($file) = @_;

  # WARNING: Gross hack: $FH is a typeglob: use its hash slot to store
  # the `name' of the file we are opening.  See the example with
  # io_socket_timeout in IO::Socket for more, and read Graham's
  # comment in IO::Handle.
  ${*$fh}{'autom4te_xfile_file'} = "$file";

  if (!$fh->SUPER::open (@_))
    {
      fatal "cannot open $file: $!";
    }

  # In case we're running under MSWindows, don't write with CRLF.
  # (This circumvents a bug in at least Cygwin bash where the shell
  # parsing fails on lines ending with the continuation character '\'
  # and CRLF).
  binmode $fh if $file =~ /^\s*>/;
}

=item C<$fh-E<gt>close>

Close the file, handling errors.

=cut

sub close
{
  my $fh = shift;
  if (!$fh->SUPER::close (@_))
    {
      my $file = $fh->name;
      Autom4te::FileUtils::handle_exec_errors $file
	unless $!;
      fatal "cannot close $file: $!";
    }
}

=item C<$line = $fh-E<gt>getline>

Read and return a line from the file.  Ensure C<\r\n> is translated to
C<\n> on input files.

=cut

# Some Win32/perl installations fail to translate \r\n to \n on input
# so we do that here.
sub getline
{
  local $_ = $_[0]->SUPER::getline;
  # Perform a _global_ replacement: $_ may can contains many lines
  # in slurp mode ($/ = undef).
  s/\015\012/\n/gs if defined $_;
  return $_;
}

=item C<@lines = $fh-E<gt>getlines>

Slurp lines from the files.

=cut

sub getlines
{
  my @res = ();
  my $line;
  push @res, $line while $line = $_[0]->getline;
  return @res;
}

=item C<$name = $fh-E<gt>name>

Return the name of the file.

=cut

sub name
{
  my $fh = shift;
  return ${*$fh}{'autom4te_xfile_file'};
}

=item C<$fh-E<gt>lock>

Lock the file using C<flock>.  If locking fails for reasons other than
C<flock> being unsupported, then error out if C<$ENV{'MAKEFLAGS'}> indicates
that we are spawned from a parallel C<make>.

=cut

sub lock
{
  my ($fh, $mode) = @_;
  # Cannot use @_ here.

  # Unless explicitly configured otherwise, Perl implements its `flock' with the
  # first of flock(2), fcntl(2), or lockf(3) that works.  These can fail on
  # NFS-backed files, with ENOLCK (GNU/Linux) or EOPNOTSUPP (FreeBSD); we
  # usually ignore these errors.  If $ENV{MAKEFLAGS} suggests that a parallel
  # invocation of `make' has invoked the tool we serve, report all locking
  # failures and abort.
  #
  # On Unicos, flock(2) and fcntl(2) over NFS hang indefinitely when `lockd' is
  # not running.  NetBSD NFS clients silently grant all locks.  We do not
  # attempt to defend against these dangers.
  #
  # -j is for parallel BSD make, -P is for parallel HP-UX make.
  if (!flock ($fh, $mode))
    {
      my $make_j = (exists $ENV{'MAKEFLAGS'}
		    && " -$ENV{'MAKEFLAGS'}" =~ / (-[BdeikrRsSw]*[jP]|--[jP]|---?jobs)/);
      my $note = "\nforgo `make -j' or use a file system that supports locks";
      my $file = $fh->name;

      msg ($make_j ? 'fatal' : 'unsupported',
	   "cannot lock $file with mode $mode: $!" . ($make_j ? $note : ""))
	if $make_j || !($!{ENOLCK} || $!{EOPNOTSUPP});
    }
}

=item C<$fh-E<gt>seek ($position, [$whence])>

Seek file to C<$position>.  Die if seeking fails.

=cut

sub seek
{
  my $fh = shift;
  # Cannot use @_ here.
  if (!seek ($fh, $_[0], $_[1]))
    {
      my $file = $fh->name;
      fatal "cannot rewind $file with @_: $!";
    }
}

=item C<$fh-E<gt>truncate ($len)>

Truncate the file to length C<$len>.  Die on failure.

=cut

sub truncate
{
  my ($fh, $len) = @_;
  if (!truncate ($fh, $len))
    {
      my $file = $fh->name;
      fatal "cannot truncate $file at $len: $!";
    }
}

=back

=head1 SEE ALSO

L<perlfunc>,
L<perlop/"I/O Operators">,
L<IO::File>
L<IO::Handle>
L<IO::Seekable>

=head1 HISTORY

Derived from IO::File.pm by Akim Demaille E<lt>F<akim@freefriends.org>E<gt>.

=cut

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
