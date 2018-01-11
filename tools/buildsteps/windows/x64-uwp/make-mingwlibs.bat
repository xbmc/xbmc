@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x64 store
CALL make-mingwlibs.bat build64 win10 %*
POPD
