@ECHO OFF
rem batch file to compile mingw libs via BuildSetup

rem set M$ env
call "C:\Program Files (x86)\Microsoft Visual Studio 1.0\VC\vcvarsall.bat"

rem check for mingw env
IF EXIST ..\..\project\BuildDependencies\msys\bin\sh.exe (
  rem compiles a bunch of mingw libs and not more
  echo bla>..\..\project\Win32BuildSetup\noprompt
  echo bla>..\..\project\Win32BuildSetup\makeclean
  ..\..\project\BuildDependencies\msys\bin\sh --login /xbmc/project/Win32BuildSetup/buildmingwlibs.sh
) ELSE (
  ECHO bla>errormingw
  ECHO mingw environment not found
)  