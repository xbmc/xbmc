@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x86 store
CALL bootstrap-addons %*
POPD
