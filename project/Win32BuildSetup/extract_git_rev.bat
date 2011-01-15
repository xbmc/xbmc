@echo off

rem Batch file output: %GIT_REV% variable, containing the git revision

rem Use tgit.exe of TortoiseGit if available
SET GITEXE="tgit.exe"
%GITEXE% --version > NUL 2>&1
if errorlevel 1 goto :notgit
GOTO :extract

:notgit

rem Fallback on msysgit - must be in the path
SET GITEXE="git.exe"
%GITEXE% --help > NUL 2>&1
if errorlevel 1 goto :nogit
GOTO :extract

:nogit

rem Failure - no git tool to extract information.
SET GIT_REV=Unknown
GOTO :done

:extract

set oldCurrentDir=%CD%
cd ..\..
FOR /F "tokens=1 delims= " %%A IN ('%GITEXE% rev-parse --short HEAD') DO SET GIT_REV=%%A
cd %oldCurrentDir%

:done

SET GITEXE=
