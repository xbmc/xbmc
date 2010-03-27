## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

LIBS = @LIBS@

EXTRA_DIST = plugin.xml classifiers.xsd Makefile.nmake

pluginsdir = $(libdir)/cpfile/plugins
plugindir = $(pluginsdir)/core

plugin_LTLIBRARIES = libcore.la
plugin_DATA = plugin.xml classifiers.xsd
plugin_HEADERS = core.h

libcore_la_SOURCES = core.c core.h
libcore_la_LDFLAGS = -no-undefined -module -avoid-version
