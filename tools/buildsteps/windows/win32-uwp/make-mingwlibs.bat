@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x86 store
CALL make-mingwlibs.bat win10 %*
POPD
