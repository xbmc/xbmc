@ECHO OFF

PUSHD %~dp0\..
CALL download-dependencies.bat win32
POPD
