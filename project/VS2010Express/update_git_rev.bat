@echo off

SET TEMPLATE=..\..\xbmc\win32\git_rev.tmpl
SET REV_FILE=..\..\xbmc\win32\git_rev.h
IF EXIST %REV_FILE% (
  del %REV_FILE%
)

CALL ..\Win32BuildSetup\extract_git_rev.bat

copy "%TEMPLATE%" "%REV_FILE%"

echo #define GIT_REV "%GIT_REV%" >> %REV_FILE%
