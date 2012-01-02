@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

REM Batch file output: %GIT_REV% variable, containing the git revision

REM Use tgit.exe of TortoiseGit if available
SET GITEXE="tgit.exe"
%GITEXE% --version > NUL 2>&1
IF errorlevel 9009 IF NOT errorlevel 9010 GOTO :notgit
GOTO :extract

:notgit

REM Fallback on msysgit - must be in the path
SET GITEXE="git.exe"
%GITEXE% --help > NUL 2>&1
IF errorlevel 9009 IF NOT errorlevel 9010 GOTO :nomsysgit
GOTO :extract

:nomsysgit

REM Fallback on wrapped msysgit - must be in the path
SET GITEXE=git.cmd
CALL %GITEXE% --help > NUL 2>&1
IF errorlevel 1 GOTO :nogit
GOTO :extract

:nogit

REM Failure - no git tool to extract information.
SET GIT_REV=Unknown
GOTO :done

:extract

SET /a counter=0
FOR /F "tokens=1-4 delims=-" %%A IN ('"%GITEXE% log --summary --abbrev=7 -n 1 --date=short --pretty=format:"%%cd-%%h""') DO (
IF "!counter!"=="0" ( SET GIT_REV=%%A%%B%%C-%%D ) ELSE ( goto exitloop )
SET /a counter+=1
)

:exitloop

echo %GIT_REV%

:done

SET GITEXE=
