@ECHO OFF

SETLOCAL

SET EXITCODE=0

rem set Visual C++ build environment
call "%VS120COMNTOOLS%..\..\VC\bin\vcvars32.bat"

SET WORKDIR=%WORKSPACE%

IF "%WORKDIR%"=="" (
  SET WORKDIR=%CD%\..\..\..
)

rem setup some paths that we need later
SET CUR_PATH=%CD%

SET BASE_PATH=%WORKDIR%\project\cmake\
SET ADDONS_PATH=%BASE_PATH%\addons
SET ADDONS_OUTPUT_PATH=%ADDONS_PATH%\output
SET ADDON_DEPENDS_PATH=%ADDONS_PATH%\depends\win32
SET ADDON_DEPENDS_BUILD_PATH=%ADDON_DEPENDS_PATH%\build

SET ERRORFILE=%BASE_PATH%\make-addon-depends.error

rem remove the output directory if it exists
IF EXIST "%ADDONS_OUTPUT_PATH%" (
	RMDIR "%ADDONS_OUTPUT_PATH%" /S /Q > NUL
)

rem create the output directory
MKDIR "%ADDONS_OUTPUT_PATH%"

rem go into the addon depends directory
CD %ADDON_DEPENDS_PATH%

rem remove the build directory if it exists
IF EXIST "%ADDON_DEPENDS_BUILD_PATH%" (
	RMDIR "%ADDON_DEPENDS_BUILD_PATH%" /S /Q > NUL
)

rem create the build directory
MKDIR "%ADDON_DEPENDS_BUILD_PATH%"

rem go into the build directory
CD "%ADDON_DEPENDS_BUILD_PATH%"

rem execute cmake to generate makefiles processable by nmake
cmake "%ADDON_DEPENDS_PATH%" -G "NMake Makefiles" -DCMAKE_INSTALL_PREFIX=%ADDONS_OUTPUT_PATH%
IF ERRORLEVEL 1 (
	ECHO cmake error level: %ERRORLEVEL% > %ERRORFILE%
	GOTO ERROR
)

rem execute nmake to build the addon depends
nmake
IF ERRORLEVEL 1 (
	ECHO nmake error level: %ERRORLEVEL% > %ERRORFILE%
	GOTO ERROR
)

rem everything was fine
GOTO END

:ERROR
rem something went wrong
ECHO Failed to build addon dependencies
ECHO See %ERRORFILE% for more details
SET EXITCODE=1

:END
rem go back to the original directory
cd %CUR_PATH%

rem exit the script with the defined exitcode
EXIT /B %EXITCODE%
