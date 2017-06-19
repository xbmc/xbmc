@ECHO OFF

rem set Visual C++ build environment
call "%VS140COMNTOOLS%..\..\VC\bin\amd64_x86\vcvarsamd64_x86.bat" || call "%VS140COMNTOOLS%..\..\VC\bin\vcvars32.bat"

PUSHD %~dp0\..
CALL make-addons.bat %*
POPD
