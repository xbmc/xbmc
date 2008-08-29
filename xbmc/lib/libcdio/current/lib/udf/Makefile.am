########################################################
# Things to make the libudf library
########################################################
#
# From libtool documentation amended with guidance from N. Boullis: 
#
#  1. Start with version information of `0:0:0' for each libtool library.
#
#  2. It is probably not a good idea to update the version information 
#     several times between public releases, but rather once per public
#     release. (This seems to be more an aesthetic consideration than
#     a hard technical one.)
#
#  3. If the library source code has changed at all since the last
#     update, then increment REVISION (`C:R:A' becomes `C:R+1:A').
#
#  4. If any interfaces have been added, removed, or changed since the
#     last update, increment CURRENT, and set REVISION to 0.
#
#  5. If any interfaces have been added since the last public release,
#     then increment AGE.
#
#  6. If any interfaces have been removed or changed since the last
#     public release, then set AGE to 0. A changed interface means an
#     incompatibility with previous versions.

libudf_la_CURRENT = 0
libudf_la_REVISION = 0
libudf_la_AGE = 0

EXTRA_DIST = libudf.sym

noinst_HEADERS = udf_fs.h udf_private.h

lib_LTLIBRARIES = libudf.la

libudf_la_SOURCES = udf.c udf_file.c udf_fs.c udf_time.c filemode.c

libudf_la_LIBADD = @LIBCDIO_LIBS@

INCLUDES = $(LIBCDIO_CFLAGS)
