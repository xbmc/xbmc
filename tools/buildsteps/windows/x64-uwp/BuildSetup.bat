@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x64 store

SET cmakeGenerator=Visual Studio %vsver% Win64
SET TARGET_ARCHITECTURE=x64
SET cmakeProps=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%UCRTVersion%

CALL BuildSetup.bat %*
POPD
