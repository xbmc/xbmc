@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x64 store
CALL bootstrap-addons %*
POPD
