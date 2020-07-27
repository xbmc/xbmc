@ECHO OFF

PUSHD %~dp0\..

CALL vswhere.bat x86
IF ERRORLEVEL 1 (
  ECHO ERROR! BuildSetup.bat: Something went wrong when calling vswhere.bat
  POPD
  EXIT /B 1
)

SET cmakeGenerator=Visual Studio %vsver%
SET cmakeArch=Win32
SET TARGET_ARCHITECTURE=x86
SET TARGET_PLATFORM=%TARGET_ARCHITECTURE%

CALL BuildSetup.bat %*
POPD
