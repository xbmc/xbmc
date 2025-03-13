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

SET vcvars=no
SET vsver=

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

FOR /F "tokens=1 delims= " %%a IN ("%vsver%") DO (
  SET vsvernumber=%%a
)

SET arch=%1
SET vcstore=%2
SET sdkver=
SET vcarch=

rem PROCESSOR_ARCHITECTURE has possible options of x86, AMD64, ARM64
rem vcvarsall.bat recognises x64 interchangebly with AMD64
rem we set vcarch for AMD64 to x64 to work with our expected value
if "%PROCESSOR_ARCHITECTURE%" == "AMD64" (
  SET vcarch=x64
) else (
  rem arm host tools are only really working in VS 17 2022+
  rem fall back to x64 for older VS installations
  if "%vsvernumber%" GEQ "17" (
    rem PROCESSOR_ARCHITECTURE returns uppercase. Use powershell to
    rem lowercase for comparison and usage with vcvarsall.bat
    FOR /F "usebackq tokens=*" %%A IN (`powershell.exe -Command "('%PROCESSOR_ARCHITECTURE%').ToLower( )"`) DO SET vcarch=%%A
  ) else (
    SET vcarch=x64
  )
)

IF "%arch%" NEQ "%vcarch%" (
  SET vcarch=%vcarch%_%arch%
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
