@echo OFF

pushd %~dp0\..\..\..
set WORKSPACE=%CD%
popd

cmake -G "%TARGET_CMAKE_GENERATOR%" -A %TARGET_CMAKE_GENERATOR_PLATFORM% -T host=x64 %TARGET_CMAKE_OPTIONS% ^
	-B %WORKSPACE%\tools\windows\depends\build-%TARGET_PLATFORM% ^
	-DCMAKE_CONFIGURATION_TYPES:STRING=%BUILD_TYPE% ^
	-DCMAKE_INSTALL_PREFIX="%WORKSPACE%\tools\depends\xbmc-depends\%TARGET_PLATFORM%" ^
	-DCMAKE_PREFIX_PATH="%WORKSPACE%\tools\depends\xbmc-depends\%TARGET_PLATFORM%" ^
	-DCMAKE_SYSROOT="%WORKSPACE%\tools\depends\xbmc-depends\%NATIVE_PLATFORM%" ^
	-DBUILD_DIR="%WORKSPACE%\tools\windows\depends\build-%TARGET_PLATFORM%\build" ^
	-DADDON_DEPENDS_PATH="%WORKSPACE%\tools\depends\xbmc-depends\%TARGET_PLATFORM%" ^
	-DTARBALL_DIR="%WORKSPACE%\tools\depends\xbmc-depends\downloads" ^
	%TARGET_CMAKE_PROPS% ^
	%WORKSPACE%\tools\windows\depends\target || exit /b

cmake --build %WORKSPACE%\tools\windows\depends\build-%TARGET_PLATFORM% --config %BUILD_TYPE% || exit /b
