#
# PortAudioCpp V19 autoconf input file
# Shamelessly ripped from the PortAudio one by Dominic Mazzoni
# Ludwig Schwardt
# Customized for automake by Mikael Magnusson
#

# Require autoconf >= 2.13
AC_PREREQ(2.13)

m4_define([lt_current], [0])
m4_define([lt_revision], [12])
m4_define([lt_age], [0])

AC_INIT([PortAudioCpp], [12])
AC_CONFIG_SRCDIR([include/portaudiocpp/PortAudioCpp.hxx])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

###### Top-level directory of pacpp
###### This makes it easy to shuffle the build directories
###### Also edit AC_CONFIG_SRCDIR above (wouldn't accept this variable)!
PACPP_ROOT="\$(top_srcdir)"
PORTAUDIO_ROOT="../.."

# Various other variables and flags
DEFAULT_INCLUDES="-I$PACPP_ROOT/include -I$PACPP_ROOT/$PORTAUDIO_ROOT/include"
CFLAGS=${CFLAGS-"-g -O2 -Wall -ansi -pedantic"}
CXXFLAGS=${CXXFLAGS-"${CFLAGS}"}

LT_VERSION_INFO="lt_current:lt_revision:lt_age"

# Checks for programs

AC_PROG_CC
AC_PROG_CXX
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# Transfer these variables to the Makefile
AC_SUBST(DEFAULT_INCLUDES)
AC_SUBST(PORTAUDIO_ROOT)
AC_SUBST(CXXFLAGS)
AC_SUBST(LT_VERSION_INFO)

AC_CONFIG_FILES([
                 Makefile
                 lib/Makefile
                 include/Makefile
                 bin/Makefile
                 doc/Makefile
                 portaudiocpp.pc
                ])
AC_OUTPUT
