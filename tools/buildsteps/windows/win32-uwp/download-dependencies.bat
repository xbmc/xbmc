@ECHO OFF

PUSHD %~dp0\..
CALL download-dependencies.bat win32-uwp
POPD
