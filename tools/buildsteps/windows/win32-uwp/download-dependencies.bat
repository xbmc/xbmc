@ECHO OFF

PUSHD %~dp0\..
CALL download-dependencies.bat win10-win32
POPD
