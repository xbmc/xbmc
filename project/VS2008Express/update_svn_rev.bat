@echo off
rem subwcrev is included in the tortoise svn client: http://tortoisesvn.net/downloads
SET REV_FILE=..\..\xbmc\win32\svn_rev.h
IF EXIST %REV_FILE% (
  del %REV_FILE%
)
subwcrev ../.. ../../xbmc/win32/svn_rev.tmpl %REV_FILE%
SET REV_FILE=
