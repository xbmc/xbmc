@ECHO OFF

PUSHD %~dp0\..

CALL vswhere.bat arm store
IF ERRORLEVEL 1 (
  ECHO ERROR! BuildSetup.bat: Something went wrong when calling vswhere.bat
  POPD
  EXIT /B 1
)

SET cmakeGenerator=Visual Studio %vsver%
SET cmakeArch=ARM
SET TARGET_ARCHITECTURE=arm
SET TARGET_PLATFORM=%TARGET_ARCHITECTURE%-uwp
SET cmakeProps=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0

CALL BuildSetup.bat %*
POPD
