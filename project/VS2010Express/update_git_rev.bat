@echo off

SET TEMPLATE=..\..\xbmc\win32\git_rev.tmpl
SET REV_FILE=..\..\xbmc\win32\git_rev.h
IF EXIST %REV_FILE% (
  del %REV_FILE%
)

rem Use tgit.exe of TortoiseGit if available
SET GITEXE="tgit.exe"
%GITEXE% --version > NUL 2>&1
if errorlevel 1 goto :notgit
GOTO :extract

:notgit

rem Fallback on msysgit, which must have been added manually to the path.
SET GITEXE="git.exe"
%GITEXE% --help > NUL 2>&1
if errorlevel 1 goto :nogit
GOTO :extract

:nogit

rem Failure - no git tool to extract information.
SET GIT_REV=Unknown
GOTO :extracted

:extract

set oldCurrentDir=%CD%
cd ..\..
FOR /F "tokens=1 delims= " %%A IN ('%GITEXE% rev-parse --short HEAD') DO SET GIT_REV=%%A
cd %oldCurrentDir%

:extracted

copy "%TEMPLATE%" "%REV_FILE%"

echo #define GIT_REV "%GIT_REV%" >> %REV_FILE%
