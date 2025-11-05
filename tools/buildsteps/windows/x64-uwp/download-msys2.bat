@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x64 store
IF ERRORLEVEL 1 (
  ECHO ERROR! download-msys2.bat: Something went wrong when calling vswhere.bat
  POPD
  EXIT /B 1
)
CALL download-msys2.bat %*
POPD
