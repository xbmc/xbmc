@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x86
CALL make-addons.bat %*
POPD
