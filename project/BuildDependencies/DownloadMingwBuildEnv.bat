@ECHO OFF

PUSHD %~dp0\..\..\tools\buildsteps\windows\win32
CALL download-msys2.bat %*
POPD
