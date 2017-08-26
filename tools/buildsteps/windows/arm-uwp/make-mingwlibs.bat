@ECHO OFF

PUSHD %~dp0\..
CALL make-mingwlibs.bat buildArm win10 %*
POPD
