@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x64
IF ERRORLEVEL 1 (
  ECHO ERROR! run-tests.bat: Something went wrong when calling vswhere.bat
  POPD
  EXIT /B 1
)
SET TARGET_PLATFORM=x64

CALL run-tests.bat
POPD
