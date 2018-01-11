@ECHO OFF

PUSHD %~dp0\..
call vswhere.bat arm store
CALL make-addons.bat win10 %*
POPD
