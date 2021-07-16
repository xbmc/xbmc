@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

REM setup all paths
PUSHD %~dp0\..\..\..
SET WORKSPACE=%CD%
POPD
cd %WORKSPACE%\kodi-build.%TARGET_PLATFORM%
set sed_exe=%WORKSPACE%\tools\depends\xbmc-depends\%NATIVE_PLATFORM%\msys2\usr\bin\sed.exe

REM read the version values from version.txt
FOR /f "tokens=2*" %%i IN ('findstr /b /c:"APP_NAME " %WORKSPACE%\version.txt') DO SET APP_NAME=%%i

CLS
COLOR 1B
TITLE %APP_NAME% testsuite Build-/Runscript

rem -------------------------------------------------------------
rem  CONFIG START
SET exitcode=0
SET useshell=sh
SET PreferredToolArchitecture=x64


  rem  CONFIG END
  rem -------------------------------------------------------------

echo Building %BUILD_TYPE%
IF EXIST buildlog.html del buildlog.html /q

ECHO Compiling testsuite...
cmake.exe --build . --config "%BUILD_TYPE%" --target %APP_NAME%-test

IF %errorlevel%==1 (
  set DIETEXT="%APP_NAME%-test.exe failed to build!  See %CD%\..\vs2010express\XBMC\%BUILD_TYPE%\objs\XBMC.log"
  type "%CD%\..\vs2010express\XBMC\%BUILD_TYPE%\objs\XBMC.log"
  goto DIE
)
ECHO Done building!
ECHO ------------------------------------------------------------

:RUNTESTSUITE
ECHO Running testsuite...
  "%BUILD_TYPE%\%APP_NAME%-test.exe" --gtest_output=xml:%WORKSPACE%\gtestresults.xml

  rem Adapt gtest xml output to be conform with junit xml
  rem this basically looks for lines which have "notrun" in the <testcase /> tag
  rem and adds a <skipped/> subtag into it. For example:
  rem <testcase name="IsStarted" status="notrun" time="0" classname="TestWebServer"/>
  rem becomes
  rem <testcase name="IsStarted" status="notrun" time="0" classname="TestWebServer"><skipped/></testcase>
  %sed_exe% "s/<testcase\(.*\)\"notrun\"\(.*\)\/>$/<testcase\1\"notrun\"\2><skipped\/><\/testcase>/" %WORKSPACE%\gtestresults.xml > %WORKSPACE%\gtestresults-skipped.xml
  del %WORKSPACE%\gtestresults.xml
  move %WORKSPACE%\gtestresults-skipped.xml %WORKSPACE%\gtestresults.xml
ECHO Done running testsuite!
ECHO ------------------------------------------------------------
GOTO END

:DIE
  ECHO ------------------------------------------------------------
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  ECHO    ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  set DIETEXT=ERROR: %DIETEXT%
  echo %DIETEXT%
  SET exitcode=1
  ECHO ------------------------------------------------------------

:END
  EXIT /B %exitcode%
