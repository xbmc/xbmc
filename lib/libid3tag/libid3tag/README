
 libid3tag - ID3 tag manipulation library
 Copyright (C) 2000-2004 Underbit Technologies, Inc.

 $Id: README,v 1.5 2004/01/23 09:41:32 rob Exp $

===============================================================================

INTRODUCTION

  libid3tag is a library for reading and (eventually) writing ID3 tags, both
  ID3v1 and the various versions of ID3v2.

  See the file `id3tag.h' for the current library interface.

  This package uses GNU libtool to arrange for zlib to be linked
  automatically when you link your programs with this library. If you aren't
  using GNU libtool, in some cases you may need to link with zlib
  explicitly:

      ${link_command} ... -lid3tag -lz

===============================================================================

BUILDING AND INSTALLING

  Note that this library depends on zlib 1.1.4 or later. If you don't have
  zlib already, you can obtain it from:

      http://www.gzip.org/zlib/

  You must have zlib installed before you can build this package.

Windows Platforms

  libid3tag can be built under Windows using either MSVC++ or Cygwin. A
  MSVC++ project file can be found under the `msvc++' subdirectory.

  To build libid3tag using Cygwin, you will first need to install the Cygwin
  tools:

      http://www.cygwin.com/

  You may then proceed with the following POSIX instructions within the
  Cygwin shell.

  Note that by default Cygwin will build a library that depends on the
  Cygwin DLL. You can use MinGW to build a library that does not depend on
  the Cygwin DLL. To do so, give the option --host=mingw32 to `configure'.
  Be certain you also link with a MinGW version of zlib.

POSIX Platforms (including Cygwin)

  The code is distributed with a `configure' script that will generate for
  you a `Makefile' and a `config.h' for your platform. See the file
  `INSTALL' for generic instructions.

  The specific options you may want to give `configure' are:

      --disable-debugging       do not compile with debugging support, and
                                use more optimizations

      --disable-shared          do not build a shared library

  By default the package will build a shared library if possible for your
  platform. If you want only a static library, use --disable-shared.

  If zlib is installed in an unusual place or `configure' can't find it, you
  may need to indicate where it is:

      ./configure ... CPPFLAGS="-I${include_dir}" LDFLAGS="-L${lib_dir}"

  where ${include_dir} and ${lib_dir} are the locations of the installed
  header and library files, respectively.

Experimenting and Developing

  Further options for `configure' that may be useful to developers and
  experimenters are:

      --enable-debugging        enable diagnostic debugging support and
                                debugging symbols

      --enable-profiling        generate `gprof' profiling code

===============================================================================

COPYRIGHT

  Please read the `COPYRIGHT' file for copyright and warranty information.
  Also, the file `COPYING' contains the full text of the GNU GPL.

  Send inquiries, comments, bug reports, suggestions, patches, etc. to:

      Underbit Technologies, Inc. <support@underbit.com>

  See also the MAD home page on the Web:

      http://www.underbit.com/products/mad/

===============================================================================

