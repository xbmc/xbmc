REM quick and dirty build file for testing different MSDEVs
setlocal 

set myFLAGS= /I../include /I../mySTL /c /W3 /G6 /O2

cl %myFLAGS% test.cpp

link.exe  /out:test.exe ../src/taocrypt.lib test.obj advapi32.lib

