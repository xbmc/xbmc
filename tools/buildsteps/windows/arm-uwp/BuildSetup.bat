@ECHO OFF

PUSHD %~dp0\..

CALL vswhere.bat arm store
IF ERRORLEVEL 1 (
  ECHO ERROR! BuildSetup.bat: Something went wrong when calling vswhere.bat
  POPD
  EXIT /B 1
)

SET cmakeGenerator=Visual Studio %vsver% ARM
SET TARGET_ARCHITECTURE=arm
SET cmakeProps=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%UCRTVersion%

CALL BuildSetup.bat %*
POPD
