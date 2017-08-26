@ECHO OFF

rem set Visual C++ build environment
call "%VS140COMNTOOLS%..\..\VC\bin\amd64_arm\vcvarsamd64_arm.bat" store 10.0.14393.0 || call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" arm store 10.0.14393.0

PUSHD %~dp0\..
CALL make-addons.bat %*
POPD
