@ECHO OFF

REM Batch file to download and build pvr-addons and place them in application's add-ons folder

SET CUR_DIR=%CD%
SET EXITCODE=0

SET DEPS_DIR=..\BuildDependencies
SET TMP_DIR=%DEPS_DIR%\tmp

SET LIBNAME=xbmc-pvr-addons
SET VERSION=1832e602ce6d6ce43e2abbb497879beb944ab225
SET SOURCE=%LIBNAME%
SET GIT_URL=git://github.com/opdenkamp/%LIBNAME%.git
SET SOURCE_DIR=%TMP_DIR%\%SOURCE%
SET BUILT_ADDONS_DIR=%SOURCE_DIR%\addons

REM check if MSBuild.exe is used because it requires different command line switches
IF "%msbuildemitsolution%" == "1" (
  set OPTS_EXE=%SOURCE_DIR%\project\VS2010Express\xbmc-pvr-addons.sln /t:Build /p:Configuration="Release" /property:VCTargetsPath="%MSBUILDROOT%Microsoft.Cpp\v4.0\V120\\" /m
) ELSE (
  set OPTS_EXE=%SOURCE_DIR%\project\VS2010Express\xbmc-pvr-addons.sln /build Release
)

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
REM run DownloadBuildDeps.bat of pvr-addons
CD "project\BuildDependencies"
CALL DownloadBuildDeps.bat > NUL 2>&1
CD "%CUR_DIR%"

REM build xbmc-pvr-addons.sln
ECHO Building PVR addons
"%MSBUILDROOT%12.0\bin\MSBuild.exe" %OPTS_EXE%

IF %errorlevel%==1 (
  goto fail
)

REM copy the built pvr addons into ADDONS_DIR
CD "%BUILT_ADDONS_DIR%"
SET ADDONS_DIR=..\..\..\..\Win32BuildSetup\BUILD_WIN32\addons\

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
ECHO No git command available. Unable to fetch and build pvr-addons.
SET EXITCODE=1

:fail
ECHO Failed to build one or more pvr addons
SET EXITCODE=1

:done
SET GITEXE=
EXIT /B %EXITCODE%
