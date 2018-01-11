@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x64 store
CALL make-addons.bat win10 %*
POPD
