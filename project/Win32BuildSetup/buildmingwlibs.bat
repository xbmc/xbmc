@ECHO OFF
rem batch file to compile mingw libs via BuildSetup
rem currently only working on x86 (see msys.bat)

rem set M$ env
call "%VS100COMNTOOLS%..\..\VC\bin\vcvars32.bat"
rem compiles a bunch of mingw libs and not more
..\BuildDependencies\msys\bin\rxvt -backspacekey  -sl 2500 -sr -fn Courier-12 -tn msys -geometry 120x25 -title "building mingw dlls" -e /bin/sh --login /xbmc/project/Win32BuildSetup/buildmingwlibs.sh