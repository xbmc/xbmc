@ECHO OFF

PUSHD %~dp0\..
call vswhere.bat arm store
CALL bootstrap-addons %*
POPD
