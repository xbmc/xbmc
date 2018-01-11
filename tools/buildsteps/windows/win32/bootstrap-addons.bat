@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x86
CALL bootstrap-addons %*
POPD
