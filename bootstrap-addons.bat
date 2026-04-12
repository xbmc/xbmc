@ECHO OFF

SETLOCAL

SET EXITCODE=0

SET clean=false
if "%1" == "clean" (
  SET clean=true
) ELSE (
  IF "%1" NEQ "" (
    SET REPOSITORY=%1

    IF "%2" NEQ "" (
      SET REPOSITORY_REVISION=%2
    )
  )
)

rem set Visual C++ build environment
CALL "%~dp0find-vs.bat" || EXIT /B 1
IF "%vcarch%"=="" SET vcarch=x64
call "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvarsall.bat" %vcarch% || exit /b 1

SET WORKDIR=%WORKSPACE%

IF "%WORKDIR%" == "" (
  rem resolve the relative path
  PUSHD ..\..\..
  SET WORKDIR=%CD%
  POPD
)

rem setup some paths that we need later
SET CUR_PATH=%CD%
SET BASE_PATH=%WORKDIR%\project\cmake
SET ADDONS_PATH=%BASE_PATH%\addons
SET ADDONS_BOOTSTRAP_PATH=%ADDONS_PATH%\bootstrap
SET BOOTSTRAP_BUILD_PATH=%ADDONS_PATH%\build\bootstrap
SET ADDONS_DEFINITION_PATH=%ADDONS_PATH%\addons

IF %clean% == true (
  rem remove the build directory if it exists
  IF EXIST "%BOOTSTRAP_BUILD_PATH%" (
    ECHO Cleaning build directory...
    RMDIR "%BOOTSTRAP_BUILD_PATH%" /S /Q > NUL
  )

  rem clean the addons definition path if it exists
  IF EXIST "%ADDONS_DEFINITION_PATH%" (
    ECHO Cleaning bootstrapped addons...
    RMDIR "%ADDONS_DEFINITION_PATH%" /S /Q > NUL
  )

  GOTO END
)

rem create the build directory
IF NOT EXIST "%BOOTSTRAP_BUILD_PATH%" MKDIR "%BOOTSTRAP_BUILD_PATH%"

rem create the addons definition directory
IF NOT EXIST "%ADDONS_DEFINITION_PATH%" MKDIR "%ADDONS_DEFINITION_PATH%"

rem go into the build directory
CD "%BOOTSTRAP_BUILD_PATH%"

ECHO --------------------------------------------------
ECHO Bootstrapping addons
ECHO --------------------------------------------------

rem pick a CMake generator: prefer Ninja, fall back to NMake Makefiles
SET CMAKE_GENERATOR=
WHERE ninja >NUL 2>&1 && SET CMAKE_GENERATOR=Ninja
IF "%CMAKE_GENERATOR%"=="" (
  WHERE nmake >NUL 2>&1 && SET CMAKE_GENERATOR=NMake Makefiles
)
IF "%CMAKE_GENERATOR%"=="" (
  ECHO ERROR: Neither ninja nor nmake found in PATH.
  ECHO Make sure Visual Studio C++ build tools are installed and vcvarsall.bat ran correctly.
  GOTO ERROR
)

rem execute cmake to generate build files
cmake "%ADDONS_BOOTSTRAP_PATH%" -G "%CMAKE_GENERATOR%" ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_INSTALL_PREFIX=%ADDONS_DEFINITION_PATH% ^
      -DBUILD_DIR=%BOOTSTRAP_BUILD_PATH% ^
      -DREPOSITORY_TO_BUILD="%REPOSITORY%" ^
      -DREPOSITORY_REVISION="%REPOSITORY_REVISION%"
IF ERRORLEVEL 1 (
  ECHO cmake failed
  GOTO ERROR
)

rem build (works with any generator)
cmake --build .
IF ERRORLEVEL 1 (
  ECHO build failed
  GOTO ERROR
)
rem everything was fine
GOTO END

:ERROR
rem something went wrong
ECHO Failed to bootstrap addons
SET EXITCODE=1

:END
rem go back to the original directory
cd %CUR_PATH%

rem exit the script with the defined exitcode
EXIT /B %EXITCODE%
