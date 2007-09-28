#
# Makefile for the webs package directory
#
# Copyright (c) Go Ahead, 1995-1999
#

all:	compile 

CLEANT	= cleant
DEPENDT	= dependt
PACKAGET= packaget

include make.dep
include $(TARGET)/$(OS)/make.$(CUT)

packaget:
	rm -f webs.zip
	"C:/Program Files/winzip/winzip32" -a -P webs.zip @files
	"C:/Program Files/winzip/winzip32" -a -P webs.zip @docfiles

cleant:
	rm -fr webs.zip websdoc.zip
	rm -fr web/doc/basic/* web/doc/ej/* web/doc/ws/*
	$(TCL) import.tcl clean

dependt:
	$(TCL) import.tcl import
