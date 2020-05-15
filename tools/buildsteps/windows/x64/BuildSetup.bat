@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x64
IF ERRORLEVEL 1 (
  ECHO ERROR! BuildSetup.bat: Something went wrong when calling vswhere.bat
  POPD
  EXIT /B 1
)

SET cmakeGenerator=Visual Studio %vsver% Win64
SET TARGET_ARCHITECTURE=x64
SET TARGET_PLATFORM=%TARGET_ARCHITECTURE%

CALL BuildSetup.bat %*
POPD
