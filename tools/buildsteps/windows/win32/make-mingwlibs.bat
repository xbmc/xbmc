@ECHO OFF

PUSHD %~dp0\..
CALL make-mingwlibs.bat %*
POPD
