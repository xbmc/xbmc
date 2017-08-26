@ECHO OFF

rem set Visual C++ build environment
call "%VS140COMNTOOLS%..\..\VC\bin\amd64_x86\vcvarsamd64_x86.bat" store 10.0.14393.0 || call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" x86 store 10.0.14393.0

PUSHD %~dp0\..
CALL make-addons.bat %*
POPD
