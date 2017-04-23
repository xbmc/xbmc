:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: Creates symbolic links of all Kodi dependencies into the specified directory.
::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

@ECHO OFF
SETLOCAL enabledelayedexpansion enableextensions
GOTO :Main


:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Helper functions


:: Print information about using this script
:Usage
(
   ECHO.
   ECHO %~nx0: Script for creating symlinks to directories and
   ECHO files that Kodi depends on (and expects to find via KODI_HOME^), allowing Kodi
   ECHO to run from the build directory without KODI_HOME being explicitly set.
   ECHO.
   ECHO This makes it possible to run/debug Kodi in scenarios where setting KODI_HOME
   ECHO to point to the build directory is not feasible (e.g., when running as a COM
   ECHO server^).
   ECHO.
   ECHO Usage:
   ECHO   %~nx0 ^<target^>
   ECHO.
   ECHO      ^<target^> The directory into which the DLLs and directories should be
   ECHO               linked. This should be the directory containing Kodi.exe.
   EXIT /B
)


:: Helper function to get the absolute path from its second argument, and set
:: that into the variable named by its first argument
:SetAbsolutePath <resultVar> <pathVar>
(
   SET "%~1=%~f2"
   EXIT /B
)


:: Checks for the existence of a symbolic link in TARGET_DIR, pointing to
:: <sourcePath>, and creates it if it does not exist
:MakeSymlink <sourcePath>
(
   SET SOURCE=%1
   SET TARGET=%TARGET_DIR%\%~nx1
   IF EXIST "%TARGET%" (
      ECHO Symlink %TARGET% exists
   ) ELSE (
      IF EXIST %SOURCE%\* (
         ECHO Creating DIR symlink %TARGET%
         mklink /D "%TARGET%" "%SOURCE%"
      ) ELSE (
         ECHO Creating FILE symlink %TARGET%
         mklink "%TARGET%" "%SOURCE%"
      )
   )
   EXIT /B
)



:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Start of main script body
:Main

:: Check for a single /? argument...this means "help"
IF "%1" == "/?" ( Call :Usage && EXIT /B )

:: Where we want to make the symlinks
IF "%1" == "" ( ECHO No target directory specified. Run %~nx0 /? for help. && EXIT /B )
IF NOT EXIST %1 ( ECHO Target directory "%1" does not exist. && EXIT /B )
CALL :SetAbsolutePath TARGET_DIR %1
ECHO Symlinking DLLs into %TARGET_DIR%

:: Root of the Kodi project repo
CALL :SetAbsolutePath KODI_ROOT %~dp0\..\..
ECHO Project root is %KODI_ROOT%

:: Symlink the DLLs in Win32BuildSetup\dependencies into %TARGET_DIR%
FOR %%f IN (%KODI_ROOT%\project\Win32BuildSetup\dependencies\*.dll) DO CALL :MakeSymlink %%f

:: Symlink "special" directories into %TARGET_DIR%
CALL :MakeSymlink %KODI_ROOT%\addons
CALL :MakeSymlink %KODI_ROOT%\language
CALL :MakeSymlink %KODI_ROOT%\media
CALL :MakeSymlink %KODI_ROOT%\sounds
CALL :MakeSymlink %KODI_ROOT%\system
CALL :MakeSymlink %KODI_ROOT%\userdata

