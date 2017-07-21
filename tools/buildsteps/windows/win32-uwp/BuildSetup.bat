@ECHO OFF

SET cmakeGenerator=Visual Studio 14
SET TARGET_ARCHITECTURE=x86
SET cmakeProps=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0.14393.0

rem set Visual C++ build environment for binary addons
call "%VS140COMNTOOLS%..\..\VC\bin\amd64_x86\vcvarsamd64_x86.bat" store 10.0.14393.0 || call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" x86 store 10.0.14393.0

PUSHD %~dp0\..
CALL BuildSetup.bat %*
POPD
