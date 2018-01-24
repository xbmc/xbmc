@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x86 store
CALL make-addons.bat win10 %*
POPD
