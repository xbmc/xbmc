@ECHO OFF

IF "%1"=="" (
  ECHO ERROR! vswhere.bat: architecture not specified
  EXIT /B 1
)

REM running vcvars more than once can cause problems; exit early if using the same configuration, error if different
IF "%VSWHERE_SET%"=="%*" (
  ECHO vswhere.bat: VC vars already configured for %VSWHERE_SET%
  GOTO :EOF
)
IF "%VSWHERE_SET%" NEQ "" (
  ECHO ERROR! vswhere.bat: VC vars are configured for %VSWHERE_SET%
  EXIT /B 1
)

REM Trick to make the path absolute
PUSHD %~dp0\..\..\..\project\BuildDependencies
SET builddeps=%CD%
POPD

SET arch=%1
SET vcarch=amd64
SET vcstore=%2
SET vcvars=no
SET sdkver=

SET vsver=
REM Current tools are only using x86/win32
REM ToDo: allow to set NATIVEPLATFORM to allow native tools based on actual native arch (eg x86/x86_64/arm/arm64)
SET toolsdir=win32

IF "%arch%" NEQ "x64" (
  SET vcarch=%vcarch%_%arch%
)

SET vswhere="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

FOR /f "usebackq tokens=1* delims=" %%i in (`%vswhere% -latest -property installationPath`) do (
  IF EXIST "%%i\VC\Auxiliary\Build\vcvarsall.bat" (
    SET vcvars="%%i\VC\Auxiliary\Build\vcvarsall.bat"
    SET vsver=15 2017
    ECHO %%i | findstr "2019" >NUL 2>NUL
    IF NOT ERRORLEVEL 1 SET vsver=16 2019
    ECHO %%i | findstr "2022" >NUL 2>NUL
    IF NOT ERRORLEVEL 1 SET vsver=17 2022
  )
)

IF %vcvars%==no (
  FOR /f "usebackq tokens=1* delims=" %%i in (`%vswhere% -legacy -property installationPath`) do (
    ECHO %%i | findstr "14" >NUL 2>NUL
    IF NOT ERRORLEVEL 1 (
      IF EXIST "%%i\VC\vcvarsall.bat" (
        SET vcvars="%%i\VC\vcvarsall.bat"
        SET vsver=14 2015
      )
    )
  )
)

IF %vcvars%==no (
  ECHO "ERROR! Could not find vcvarsall.bat"
  EXIT /B 1
)

REM vcvars changes the cwd so we need to store it and restore it
PUSHD %~dp0
CALL %vcvars% %vcarch% %vcstore% %sdkver%
POPD

IF ERRORLEVEL 1 (
  ECHO "ERROR! something went wrong when calling"
  ECHO %vcvars% %vcarch% %vcstore% %sdkver%
  EXIT /B 1
)

SET VSWHERE_SET=%*
