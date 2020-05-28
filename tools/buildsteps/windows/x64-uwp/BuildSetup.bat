@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x64 store
IF ERRORLEVEL 1 (
  ECHO ERROR! BuildSetup.bat: Something went wrong when calling vswhere.bat
  POPD
  EXIT /B 1
)

SET cmakeGenerator=Visual Studio %vsver% Win64
SET TARGET_ARCHITECTURE=x64
SET TARGET_PLATFORM=%TARGET_ARCHITECTURE%-uwp
SET cmakeProps=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%UCRTVersion%

CALL BuildSetup.bat %*
POPD
