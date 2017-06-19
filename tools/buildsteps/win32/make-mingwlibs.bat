@ECHO OFF

PUSHD %~dp0\..\windows\win32
CALL make-mingwlibs.bat %*
POPD
