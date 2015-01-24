@echo off

<<<<<<< HEAD
SET REV_FILE=..\..\git_revision.h

CALL ..\Win32BuildSetup\extract_git_rev.bat

IF NOT [%GIT_REV%]==[] (
  echo #define GIT_REV "%GIT_REV%" > "%REV_FILE%"
) ELSE (
  echo. > "%REV_FILE%"
)
=======
SET TEMPLATE=..\..\xbmc\win32\git_rev.tmpl
SET REV_FILE=..\..\xbmc\win32\git_rev.h
IF EXIST %REV_FILE% (
  del %REV_FILE%
)

CALL ..\Win32BuildSetup\extract_git_rev.bat

copy "%TEMPLATE%" "%REV_FILE%"

echo #define GIT_REV "%GIT_REV%" >> %REV_FILE%
>>>>>>> FETCH_HEAD
