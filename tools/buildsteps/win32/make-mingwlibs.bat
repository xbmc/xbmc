@ECHO OFF
rem batch file to compile mingw libs via BuildSetup

rem set M$ env
call "%VS100COMNTOOLS%..\..\VC\bin\vcvars32.bat"

SET PROMPTLEVEL=prompt
SET BUILDMODE=clean
SET opt=rxvt
FOR %%b in (%1, %2, %3) DO (
  IF %%b==noprompt SET PROMPTLEVEL=noprompt
  IF %%b==clean SET BUILDMODE=clean
  IF %%b==noclean SET BUILDMODE=noclean
  IF %%b==sh SET opt=sh
)

SET BS_DIR=%WORKSPACE%\project\Win32BuildSetup
cd %BS_DIR%

IF EXIST errormingw del errormingw > NUL

rem compiles a bunch of mingw libs and not more
IF %opt%==sh (
  IF EXIST %WORKSPACE%\project\BuildDependencies\msys\bin\sh.exe (
    ECHO starting sh shell
    %WORKSPACE%\project\BuildDependencies\msys\bin\sh --login /xbmc/tools/buildsteps/win32/make-mingwlibs.sh
    GOTO END
  ) ELSE (
    GOTO ENDWITHERROR
  )
)
IF EXIST %WORKSPACE%\project\BuildDependencies\msys\bin\rxvt.exe (
  ECHO starting rxvt shell
  %WORKSPACE%\project\BuildDependencies\msys\bin\rxvt -backspacekey  -sl 2500 -sr -fn Courier-12 -tn msys -geometry 120x25 -title "building mingw dlls" -e /bin/sh --login /xbmc//xbmc/tools/buildsteps/win32/make-mingwlibs.sh
  GOTO END
)
GOTO ENDWITHERROR

:ENDWITHERROR
  ECHO msys environment not found
  EXIT /B 1
  
:END
  ECHO exiting msys environment
  IF EXIST errormingw (
    ECHO failed to build mingw libs
	del errormingw > NUL
    EXIT /B 1
  )
  EXIT /B 0