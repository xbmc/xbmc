@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x64

SET cmakeGenerator=Visual Studio %vsver% Win64
SET TARGET_ARCHITECTURE=x64

CALL BuildSetup.bat %*
POPD
