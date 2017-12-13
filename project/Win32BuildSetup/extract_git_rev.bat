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

FOR /F %%A IN ('"%GITEXE% --no-pager show -s --abbrev=8 --date=format:"%%Y%%m%%d" --pretty=format:"%%cd-%%h""') DO SET GIT_REV=%%A


echo %GIT_REV%

:done

SET GITEXE=
