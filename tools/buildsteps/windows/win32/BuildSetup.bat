@ECHO OFF

PUSHD %~dp0\..

CALL vswhere.bat x86

SET cmakeGenerator=Visual Studio %vsver%
SET TARGET_ARCHITECTURE=x86

CALL BuildSetup.bat %*
POPD
