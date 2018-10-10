@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x86 store
IF ERRORLEVEL 1 (
  ECHO ERROR! BuildSetup.bat: Something went wrong when calling vswhere.bat
  POPD
  EXIT /B 1
)

SET cmakeGenerator=Visual Studio %vsver%
SET TARGET_ARCHITECTURE=x86
SET cmakeProps=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%UCRTVersion%

CALL BuildSetup.bat %*
POPD
