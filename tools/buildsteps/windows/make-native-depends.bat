@echo OFF

call %~dp0\.helpers\default.bat

pushd %~dp0\..\..\..
set WORKSPACE=%CD%
popd

cmake -G "%NATIVE_CMAKE_GENERATOR%" -A %NATIVE_CMAKE_GENERATOR_PLATFORM% -T host=x64 ^
	-B %WORKSPACE%\tools\windows\depends\build-%NATIVE_PLATFORM% ^
	-DCMAKE_CONFIGURATION_TYPES:STRING=Release ^
	-DCMAKE_INSTALL_PREFIX="%WORKSPACE%\tools\depends\xbmc-depends\%NATIVE_PLATFORM%" ^
	-DCMAKE_PREFIX_PATH="%WORKSPACE%\tools\depends\xbmc-depends\%NATIVE_PLATFORM%" ^
	-DBUILD_DIR="%WORKSPACE%\tools\windows\depends\build-%NATIVE_PLATFORM%\build" ^
	-DADDON_DEPENDS_PATH="%WORKSPACE%\tools\depends\xbmc-depends\%NATIVE_PLATFORM%" ^
	-DTARBALL_DIR="%WORKSPACE%\tools\depends\xbmc-depends\downloads" ^
	%WORKSPACE%\tools\windows\depends\native || exit /b

cmake --build %WORKSPACE%\tools\windows\depends\build-%NATIVE_PLATFORM% --config Release || exit /b
