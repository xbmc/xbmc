@ECHO OFF

PUSHD %~dp0\..

CALL vswhere.bat arm store

SET cmakeGenerator=Visual Studio %vsver% ARM
SET TARGET_ARCHITECTURE=arm
SET cmakeProps=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%UCRTVersion%

CALL BuildSetup.bat %*
POPD
