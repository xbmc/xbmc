## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

LIBS = @LIBS@

CPPFLAGS = -I$(srcdir)/../core @CPPFLAGS@

EXTRA_DIST = plugin.xml Makefile.nmake

pluginsdir = $(libdir)/cpfile/plugins
plugindir = $(pluginsdir)/special

plugin_LTLIBRARIES = libspecial.la
plugin_DATA = plugin.xml

libspecial_la_SOURCES = special.c
libspecial_la_LDFLAGS = -no-undefined -module -avoid-version
