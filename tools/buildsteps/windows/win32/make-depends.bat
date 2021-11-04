@echo OFF

call %~dp0\default.bat
call %~dp0\..\make-depends.bat || exit /b
