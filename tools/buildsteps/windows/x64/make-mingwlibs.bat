@ECHO OFF

call %~dp0\default.bat
PUSHD %~dp0\..
CALL make-mingwlibs.bat build64 %*
POPD
