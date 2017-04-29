@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

REM setup all paths
PUSHD %~dp0\..\..\..
SET WORKSPACE=%CD%
POPD
cd %WORKSPACE%\kodi-build
SET builddeps_dir=%WORKSPACE%\project\BuildDependencies
SET msys_dir=%builddeps_dir%\msys64
IF NOT EXIST %msys_dir% (SET msys_dir=%builddeps_dir%\msys32)
SET awk_exe=%msys_dir%\usr\bin\awk.exe
SET sed_exe=%msys_dir%\usr\bin\sed.exe

REM read the version values from version.txt
FOR /f %%i IN ('%awk_exe% "/APP_NAME/ {print $2}" %WORKSPACE%\version.txt') DO SET APP_NAME=%%i

CLS
COLOR 1B
TITLE %APP_NAME% testsuite Build-/Runscript

rem -------------------------------------------------------------
rem  CONFIG START
SET exitcode=0
SET useshell=sh
SET BRANCH=na
SET buildconfig=Release


  :: sets the BRANCH env var
  call getbranch.bat

  rem  CONFIG END
  rem -------------------------------------------------------------

echo Building %buildconfig%
IF EXIST buildlog.html del buildlog.html /q

ECHO Compiling testsuite...
cmake.exe --build . --config "%buildconfig%" --target %APP_NAME%-test

IF %errorlevel%==1 (
  set DIETEXT="%APP_NAME%-test.exe failed to build!  See %CD%\..\vs2010express\XBMC\%buildconfig%\objs\XBMC.log"
  type "%CD%\..\vs2010express\XBMC\%buildconfig%\objs\XBMC.log"
  goto DIE
)
ECHO Done building!
ECHO ------------------------------------------------------------

:RUNTESTSUITE
ECHO Running testsuite...
  "%buildconfig%\%APP_NAME%-test.exe" --gtest_output=xml:%WORKSPACE%\gtestresults.xml

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
