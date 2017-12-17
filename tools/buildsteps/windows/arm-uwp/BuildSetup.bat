@ECHO OFF

SET cmakeGenerator=Visual Studio 14 ARM
SET TARGET_ARCHITECTURE=arm

rem set Visual C++ build environment for binary addons
call "%VS140COMNTOOLS%..\..\VC\bin\amd64_arm\vcvarsamd64_arm.bat" store 10.0.14393.0 || call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" arm store 10.0.14393.0

SET cmakeProps=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%UCRTVersion%
PUSHD %~dp0\..
CALL BuildSetup.bat %*
POPD
