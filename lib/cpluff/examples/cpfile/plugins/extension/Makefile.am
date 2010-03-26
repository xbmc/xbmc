## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

LIBS = @LIBS@

CPPFLAGS = -I$(srcdir)/../core @CPPFLAGS@

EXTRA_DIST = plugin.xml file_types.xsd Makefile.nmake

pluginsdir = $(libdir)/cpfile/plugins
plugindir = $(pluginsdir)/extension

plugin_LTLIBRARIES = libextension.la
plugin_DATA = plugin.xml file_types.xsd

libextension_la_SOURCES = extension.c
libextension_la_LDFLAGS = -no-undefined -module -avoid-version
