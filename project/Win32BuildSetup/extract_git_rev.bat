<<<<<<< HEAD
@ECHO OFF

REM Batch file output: %GIT_REV% variable, containing the git revision

SET GIT_REV=


REM Try wrapped msysgit - must be in the path
SET GITEXE=git.cmd
CALL %GITEXE% --help > NUL 2>&1
IF errorlevel 1 GOTO nowrapmsysgit
GOTO :extract

:nowrapmsysgit

REM Fallback on regular msysgit - must be in the path
SET GITEXE=git.exe
%GITEXE% --help > NUL 2>&1
IF errorlevel 9009 IF NOT errorlevel 9010 GOTO nomsysgit
GOTO :extract

:nomsysgit

REM Fallback on tgit.exe of TortoiseGit if available
SET GITEXE=tgit.exe
%GITEXE% --version > NUL 2>&1
IF errorlevel 9009 IF NOT errorlevel 9010 GOTO done
GOTO :extract


:extract

FOR /F "tokens=1-4 delims=-" %%A IN ('"%GITEXE% rev-list HEAD -n 1 --date=short --pretty=format:"%%cd-%%h""') DO (
SET GIT_REV=%%A%%B%%C-%%D
)


echo %GIT_REV%
=======
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
>>>>>>> FETCH_HEAD

:done

SET GITEXE=
