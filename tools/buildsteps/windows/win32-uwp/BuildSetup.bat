@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x86 store

SET cmakeGenerator=Visual Studio %vsver%
SET TARGET_ARCHITECTURE=x86
SET cmakeProps=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%UCRTVersion%

CALL BuildSetup.bat %*
POPD
