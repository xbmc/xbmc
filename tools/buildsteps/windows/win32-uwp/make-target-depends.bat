@echo OFF

call %~dp0\default.bat
PUSHD %~dp0\..
call make-target-depends.bat
popd
