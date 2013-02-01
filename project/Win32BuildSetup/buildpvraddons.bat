@ECHO OFF

REM Batch file to download and build xbmc-pvr-addons and place them in xbmc's addons folder

SET CUR_DIR=%CD%
SET EXITCODE=0

SET DEPS_DIR=..\BuildDependencies
SET TMP_DIR=%DEPS_DIR%\tmp

SET LIBNAME=xbmc-pvr-addons
SET VERSION=96774c4f775b156a46fb58151379dece3e773c96
SET SOURCE=%LIBNAME%
SET GIT_URL=git://github.com/opdenkamp/%LIBNAME%.git
SET SOURCE_DIR=%TMP_DIR%\%SOURCE%
SET BUILT_ADDONS_DIR=%SOURCE_DIR%\addons

set OPTS_EXE=%SOURCE_DIR%\project\VS2010Express\xbmc-pvr-addons.sln /build Release

REM Try wrapped msysgit - must be in the path
SET GITEXE=git.cmd
CALL %GITEXE% --help > NUL 2>&1
IF errorlevel 1 GOTO nowrapmsysgit
GOTO work

:nowrapmsysgit

REM Fallback on regular msysgit - must be in the path
SET GITEXE=git.exe
%GITEXE% --help > NUL
IF errorlevel 9009 IF NOT errorlevel 9010 GOTO nomsysgit
GOTO work

:nomsysgit

REM Fallback on tgit.exe of TortoiseGit if available
SET GITEXE=tgit.exe
%GITEXE% --version > NUL 2>&1
IF errorlevel 9009 IF NOT errorlevel 9010 GOTO error
GOTO work


:work
IF NOT EXIST "%TMP_DIR%" MD "%TMP_DIR%"

REM clone the git repository into SOURCE_DIR
CALL %GITEXE% clone %GIT_URL% "%SOURCE_DIR%" > NUL 2>&1
CD "%SOURCE_DIR%"
REM get the proper revision
CALL %GITEXE% checkout %VERSION% > NUL 2>&1

:build
REM run DownloadBuildDeps.bat of xbmc-pvr-addons
CD "project\BuildDependencies"
CALL DownloadBuildDeps.bat > NUL 2>&1
CD "%CUR_DIR%"

REM build xbmc-pvr-addons.sln
ECHO Building PVR addons
%1 %OPTS_EXE%

REM copy the built pvr addons into ADDONS_DIR
CD "%BUILT_ADDONS_DIR%"
SET ADDONS_DIR=..\..\..\..\Win32BuildSetup\BUILD_WIN32\Xbmc\xbmc-pvr-addons

REM exclude some files
ECHO addon.xml.in >  exclude.txt
ECHO _win32.exp   >> exclude.txt
ECHO _win32.lib   >> exclude.txt
ECHO _win32.pdb   >> exclude.txt
FOR /D %%A IN ("pvr.*") DO (
  IF EXIST "%%A\addon" (
    ECHO Installing %%A
    XCOPY "%%A\addon\*" "%ADDONS_DIR%\%%A" /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
  )
)
DEL exclude.txt > NUL
CD "%CUR_DIR%"

REM cleanup temporary directories
RMDIR "%TMP_DIR%" /S /Q > NUL

GOTO done

:error
ECHO No git command available. Unable to fetch and build xbmc-pvr-addons.
SET EXITCODE=1

:done
SET GITEXE=
EXIT /B %EXITCODE%
