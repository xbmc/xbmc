@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION
REM setup all paths
SET cur_dir=%WORKSPACE%\project\Win32BuildSetup
cd %cur_dir%
SET base_dir=%cur_dir%\..\..
SET builddeps_dir=%cur_dir%\..\..\project\BuildDependencies
SET msys_dir=%builddeps_dir%\msys64
IF NOT EXIST %msys_dir% (SET msys_dir=%builddeps_dir%\msys32)
SET awk_exe=%msys_dir%\usr\bin\awk.exe
SET sed_exe=%msys_dir%\usr\bin\sed.exe

REM read the version values from version.txt
FOR /f %%i IN ('%awk_exe% "/APP_NAME/ {print $2}" %base_dir%\version.txt') DO SET APP_NAME=%%i

CLS
COLOR 1B
TITLE %APP_NAME% testsuite Build-/Runscript

rem -------------------------------------------------------------
rem  CONFIG START
SET exitcode=0
SET useshell=sh
SET BRANCH=na
SET buildconfig=Debug Testsuite
set WORKSPACE=%CD%\..\..


  REM look for MSBuild.exe delivered with Visual Studio 2015
  FOR /F "tokens=2,* delims= " %%A IN ('REG QUERY HKLM\SOFTWARE\Microsoft\MSBuild\ToolsVersions\14.0 /v MSBuildToolsRoot') DO SET MSBUILDROOT=%%B
  SET NET="%MSBUILDROOT%14.0\bin\MSBuild.exe"

  IF EXIST "!NET!" (
    set msbuildemitsolution=1
    set OPTS_EXE="..\VS2010Express\XBMC for Windows.sln" /t:Build /p:Configuration="%buildconfig%" /property:VCTargetsPath="%MSBUILDROOT%Microsoft.Cpp\v4.0\V140" /m
    set CLEAN_EXE="..\VS2010Express\XBMC for Windows.sln" /t:Clean /p:Configuration="%buildconfig%" /property:VCTargetsPath="%MSBUILDROOT%Microsoft.Cpp\v4.0\V140"
  )

  IF NOT EXIST %NET% (
    set DIETEXT=MSBuild was not found.
    goto DIE
  )
  
  set EXE= "..\VS2010Express\XBMC\%buildconfig%\%APP_NAME%-test.exe"
  set PDB= "..\VS2010Express\XBMC\%buildconfig%\%APP_NAME%.pdb"
  
  :: sets the BRANCH env var
  call getbranch.bat

  rem  CONFIG END
  rem -------------------------------------------------------------

echo Building %buildconfig%
IF EXIST buildlog.html del buildlog.html /q

ECHO Compiling testsuite...
%NET% %OPTS_EXE%

IF %errorlevel%==1 (
  set DIETEXT="%APP_NAME%-test.exe failed to build!  See %CD%\..\vs2010express\XBMC\%buildconfig%\objs\XBMC.log"
  type "%CD%\..\vs2010express\XBMC\%buildconfig%\objs\XBMC.log"
  goto DIE
)
ECHO Done building!
ECHO ------------------------------------------------------------

:RUNTESTSUITE
ECHO Running testsuite...
  cd %WORKSPACE%\project\vs2010express\
  set KODI_HOME=%WORKSPACE%
  set PATH=%WORKSPACE%\system;%PATH%

  %EXE% --gtest_output=xml:%WORKSPACE%\gtestresults.xml

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
