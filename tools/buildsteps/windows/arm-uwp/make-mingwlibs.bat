@ECHO OFF

PUSHD %~dp0\..
call vswhere.bat arm store
CALL make-mingwlibs.bat buildArm win10 %*
POPD
