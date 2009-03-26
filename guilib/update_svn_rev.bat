@echo off
rem subwcrev is included in the tortoise svn client: http://tortoisesvn.net/downloads
SET REV_FILE=..\xbmc\xbox\svn_rev.h

IF EXIST %REV_FILE% (
  del %REV_FILE%
)

subwcrev . ../xbmc/xbox/svn_rev.tmpl %REV_FILE%

IF NOT EXIST %REV_FILE% (
  copy ..\xbmc\xbox\svn_rev.unknown %REV_FILE%
)

SET REV_FILE=