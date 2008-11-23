@echo off
REM set up a CVS tree to build when there's no autotools
REM $Revision: 1.6 $
REM $Date: 2008-08-07 00:29:08 $

REM create hugehelp.c
copy src\hugehelp.c.cvs src\hugehelp.c

REM create Makefile
copy Makefile.dist Makefile

REM create curlbuild.h
copy include\curl\curlbuild.h.dist include\curl\curlbuild.h
