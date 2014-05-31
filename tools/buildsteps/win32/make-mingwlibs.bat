@ECHO OFF
rem batch file to compile mingw libs via BuildSetup
SET WORKDIR=%WORKSPACE%
rem set M$ env
call "%VS120COMNTOOLS%vsvars32.bat" || exit /b 1

SET PROMPTLEVEL=prompt
SET BUILDMODE=clean
SET opt=rxvt
FOR %%b in (%1, %2, %3) DO (
  IF %%b==noprompt SET PROMPTLEVEL=noprompt
  IF %%b==clean SET BUILDMODE=clean
  IF %%b==noclean SET BUILDMODE=noclean
  IF %%b==sh SET opt=sh
)

IF "%WORKDIR%"=="" (
  SET WORKDIR=%CD%\..\..\..
)

SET ERRORFILE=%WORKDIR%\project\Win32BuildSetup\errormingw

SET BS_DIR=%WORKDIR%\project\Win32BuildSetup
rem cd %BS_DIR%

IF EXIST %ERRORFILE% del %ERRORFILE% > NUL

rem compiles a bunch of mingw libs and not more
IF %opt%==sh (
  IF EXIST %WORKDIR%\project\BuildDependencies\msys\bin\sh.exe (
    ECHO starting sh shell
    %WORKDIR%\project\BuildDependencies\msys\bin\sh --login /xbmc/tools/buildsteps/win32/make-mingwlibs.sh
    GOTO END
  ) ELSE (
    GOTO ENDWITHERROR
  )
)
IF EXIST %WORKDIR%\project\BuildDependencies\msys\bin\rxvt.exe (
  ECHO starting rxvt shell
  %WORKDIR%\project\BuildDependencies\msys\bin\rxvt -backspacekey  -sl 2500 -sr -fn Courier-12 -tn msys -geometry 120x25 -title "building mingw dlls" -e /bin/sh --login /xbmc/tools/buildsteps/win32/make-mingwlibs.sh
  GOTO END
)
GOTO ENDWITHERROR

:ENDWITHERROR
  ECHO msys environment not found
  ECHO bla>%ERRORFILE%
  EXIT /B 1
  
:END
  ECHO exiting msys environment
  IF EXIST %ERRORFILE% (
    ECHO failed to build mingw libs
    EXIT /B 1
  )
  EXIT /B 0