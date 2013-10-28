@ECHO OFF
rem batch file to compile mingw libs via BuildSetup

rem set M$ env
call "%VS100COMNTOOLS%..\..\VC\bin\vcvars32.bat"

SET opt=sh
IF $%1$==$$ SET opt=rxvt

rem compiles a bunch of mingw libs and not more
IF %opt%==sh (
  IF EXIST ..\BuildDependencies\msys\bin\sh.exe (
    ECHO starting sh shell
    ..\BuildDependencies\msys\bin\sh --login /xbmc/project/Win32BuildSetup/buildmingwlibs.sh
    GOTO END
  ) ELSE (
    GOTO ENDWITHERROR
  )
)
IF EXIST ..\BuildDependencies\msys\bin\rxvt.exe (
  ECHO starting rxvt shell
  ..\BuildDependencies\msys\bin\rxvt -backspacekey  -sl 2500 -sr -fn Courier-12 -tn msys -geometry 120x25 -title "building mingw dlls" -e /bin/sh --login /xbmc/project/Win32BuildSetup/buildmingwlibs.sh
  GOTO END
)
GOTO ENDWITHERROR

:ENDWITHERROR
  ECHO bla>errormingw
  ECHO msys environment not found
  
:END
  ECHO exiting msys environment