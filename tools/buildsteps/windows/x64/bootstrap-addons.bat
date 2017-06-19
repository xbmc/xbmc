@ECHO OFF

rem set Visual C++ build environment
call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" x64

PUSHD %~dp0\..
CALL bootstrap-addons %*
POPD
