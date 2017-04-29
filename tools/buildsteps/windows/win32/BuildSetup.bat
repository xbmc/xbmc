@ECHO OFF

SET cmakeGenerator=Visual Studio 14
SET TARGET_ARCHITECTURE=x86

rem set Visual C++ build environment for binary addons
call "%VS140COMNTOOLS%..\..\VC\bin\amd64_x86\vcvarsamd64_x86.bat" || call "%VS140COMNTOOLS%..\..\VC\bin\vcvars32.bat"

PUSHD %~dp0\..
CALL BuildSetup.bat %*
POPD
