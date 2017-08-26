@ECHO OFF

rem set Visual C++ build environment
call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" amd64 store 10.0.14393.0

PUSHD %~dp0\..
CALL bootstrap-addons %*
POPD
