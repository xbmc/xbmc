@ECHO OFF

REM Batch file output: %GIT_REV% variable, containing the git revision

REM Use tgit.exe of TortoiseGit if available
SET GITEXE="tgit.exe"
%GITEXE% --version > NUL 2>&1
IF errorlevel 1 GOTO :notgit
GOTO :extract

:notgit

REM Fallback on msysgit - must be in the path
SET GITEXE="git.exe"
%GITEXE% --help > NUL 2>&1
IF errorlevel 1 GOTO :nogit
GOTO :extract

:nogit

REM Failure - no git tool to extract information.
SET GIT_REV=Unknown
GOTO :done

:extract

SET oldCurrentDir=%CD%
CD ..\..
FOR /F "tokens=1-4 delims=-" %%A IN ('"%GITEXE% --no-pager log -1 --date=short --pretty=format:"%%cd-%%h""') DO SET GIT_REV=%%A%%B%%C-%%D
CD %oldCurrentDir%

:done

SET GITEXE=
