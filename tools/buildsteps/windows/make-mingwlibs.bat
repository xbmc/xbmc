@ECHO OFF

rem batch file to compile mingw libs via BuildSetup
PUSHD %~dp0\..\..\..
SET WORKDIR=%CD%
POPD

REM recreates clean ffmpeg build dir
SET BUILD_DIR=%WORKDIR%\project\BuildDependencies\build
IF EXIST %BUILD_DIR% rmdir %BUILD_DIR% /S /Q
IF NOT EXIST %BUILD_DIR% mkdir %BUILD_DIR%

SET PROMPTLEVEL=prompt
SET BUILDMODE=clean
SET opt=mintty
SET build32=yes
SET build64=no
SET buildArm=no
SET vcarch=x86
SET msys2=msys64
SET win10=no
SET TARGETPLATFORM=win32

FOR %%b in (%*) DO (
  IF %%b==noprompt SET PROMPTLEVEL=noprompt
  IF %%b==clean SET BUILDMODE=clean
  IF %%b==noclean SET BUILDMODE=noclean
  IF %%b==sh SET opt=sh
  IF %%b==build64 (
    SET build64=yes
    SET build32=no
    SET buildArm=no
    SET vcarch=amd64
    SET TARGETPLATFORM=x64
    )
  IF %%b==buildArm (
    SET build64=no
    SET build32=no
    SET buildArm=yes
    SET vcarch=arm
    SET TARGETPLATFORM=arm
    )
  IF %%b==win10 (
    SET win10=yes
  )
)
:: Export full current PATH from environment into MSYS2
set MSYS2_PATH_TYPE=inherit

REM Prepend the msys and mingw paths onto %PATH%
SET MSYS_INSTALL_PATH=%WORKDIR%\project\BuildDependencies\msys
SET PATH=%MSYS_INSTALL_PATH%\mingw\bin;%MSYS_INSTALL_PATH%\bin;%PATH%
SET ERRORFILE=%WORKDIR%\project\Win32BuildSetup\errormingw
SET BS_DIR=%WORKDIR%\project\Win32BuildSetup

IF EXIST %ERRORFILE% del %ERRORFILE% > NUL

rem compiles a bunch of mingw libs and not more
IF %opt%==sh (
  IF EXIST %WORKDIR%\project\BuildDependencies\%msys2%\usr\bin\sh.exe (
    ECHO starting sh shell
    %WORKDIR%\project\BuildDependencies\%msys2%\usr\bin\sh.exe --login -i /xbmc/tools/buildsteps/windows/make-mingwlibs.sh --prompt=%PROMPTLEVEL% --mode=%BUILDMODE% --build32=%build32% --build64=%build64% --buildArm=%buildArm% --win10=%win10%
    GOTO END
  ) ELSE (
    GOTO ENDWITHERROR
  )
)
IF EXIST %WORKDIR%\project\BuildDependencies\%msys2%\usr\bin\mintty.exe (
  ECHO starting mintty shell
  %WORKDIR%\project\BuildDependencies\%msys2%\usr\bin\mintty.exe -d -i /msys2.ico /usr/bin/bash --login /xbmc/tools/buildsteps/windows/make-mingwlibs.sh --prompt=%PROMPTLEVEL% --mode=%BUILDMODE% --build32=%build32% --build64=%build64% --buildArm=%buildArm% --win10=%win10%
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

ENDLOCAL
