@echo OFF

call %~dp0\.helpers\default.bat

pushd %~dp0\..\..\..
set WORKSPACE=%CD%
popd

call %~dp0\.helpers\pathChanged.bat %WORKSPACE%\tools\depends\xbmc-depends && exit /b

call %~dp0\make-native-depends.bat || exit /b

call %~dp0\make-target-depends.bat || exit /b

call %~dp0\.helpers\tagSuccessfulBuild.bat %WORKSPACE%\tools\depends\xbmc-depends
