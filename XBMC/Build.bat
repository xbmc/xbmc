@ECHO OFF
CLS
COLOR 1B
TITLE XBMC Build Prepare Script
rem ----PURPOSE----
rem - Create a working XBMC build with a single click
rem -------------------------------------------------------------
rem Config
rem If you get an error that Visual studio was not found, SET your path for VSNET main executable.
rem ONLY needed if you have a very old bios, SET the path for xbepatch. Not needed otherwise.
rem If Winrar isn't installed under standard programs, SET the path for WinRAR's (freeware) rar.exe
rem and finally set the options for the final rar.
rem -------------------------------------------------------------
rem Remove 'rem' from 'web / python' below to copy these to the BUILD directory.
rem -------------------------------------------------------------
rem	CONFIG START
	IF "%VS71COMNTOOLS%"=="" (
	  set NET="%ProgramFiles%\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv.com"
	) ELSE (
	  set NET="%VS71COMNTOOLS%\..\IDE\devenv.com"
	)
	IF NOT EXIST %NET% (
	  set DIETEXT=Visual Studio .NET 2003 was not found.
	  goto DIE
	) 
	set OPTS=xbmc.sln /build release
 	set CLEAN=xbmc.sln /clean release
	set XBE=release\default.xbe

  set OPTS_EXE=tools\Win32\XBMC_PC.sln /build release
	set CLEAN_EXE=tools\Win32\XBMC_PC.sln /clean release
	set EXE= tools\Win32\Release\XBMC_PC.exe
	
  set XBE_PATCH=tools\xbepatch\xbepatch.exe
	set RAR="%ProgramFiles%\Winrar\rar.exe"
	set RAR_ROOT=rar.exe
	set RAROPS=a -r -idp -inul -m5 XBMC.rar BUILD
	set RAROPS_EXE=a -r -idp -inul -m5 XBMC_PC.rar BUILD_WIN32
  rem	CONFIG END
  rem -------------------------------------------------------------

  ECHO    ВВВВВВВББББББББААААААА
  ECHO  ВлллллллллллллллллллллллВВВВВВБББББАААААА     пппВмм
  ECHO олллллллллллллллллллллллллллллллллллллллллВВВВБББАА  ппм
  ECHO ВлллллллллллллллллллллллллллллллллллллллллллллллллллВА  н
  ECHO ВлллллллллллллллллллплллллллллллллллллллллллллллллллллА В
  ECHO БллллллллллллллллллнАлллллллллллллллллллллллллллллллллл о
  ECHO АллллллВБА  плп           плллп    пВп    плллп   АВллл о
  ECHO  ллллллллллн   млллн Влллм олн мВлм   мллм ол  мллллллл о
  ECHO  Влллллллллл  лллллн лллллн л оллллн оллллн н олллллллл В
  ECHO  Блллллллллн олллллн лллллн л лллллн лллллн   ВлллллллВ н
  ECHO  АВлллллллп   пллллн плллп он лллллн лллллн А плллллллн н
  ECHO   БлллВБА мллм АБВллм     млВмллллллмлллллВ лм   АВлллно
  ECHO   АВлллллллллллллллллллллллллллллллллллллллллллллллллл В
  ECHO    Блллллпппплпплппллпллпллллпплпплппплпплпплппллллллл н
  ECHO    АВлллл но л пл л л лнмоллл лл пл л лнол пл пмлллллБ н
  ECHO     Блллл но л пл пмл л м ллл пл пл л лнол пл л лллллАо
  ECHO     АВллллллллллллллллллллллллллллллллллллллллллллллВ
  ECHO      БАммммммммммммммммммммммммммммммммммммм  АБВВВВ
  ECHO      АВллллллллллллллллллллллллллллллллллллллВААБВп
  ECHO       БВлллллллллллллллллллллллллллллВлВВпппп
  ECHO        ВВллллллллллллллллллВлВВпппп
  ECHO         пВллллВлВВВпппппп
  ECHO ------------------------------------------------------------
  ECHO XBMC prepare menu
  ECHO ------------------------------------------------------------
  ECHO [1] Build XBMC XBE     ( for XBOX use )
  ECHO [2] Build XBMC_WIN32   ( for Windows use)
  ECHO ------------------------------------------------------------
  set /P XBMC_COMPILE_ANSWER=Please enter the number you want to build! [1/2]:
  if /I %XBMC_COMPILE_ANSWER% EQU 1 goto XBE_COMPILE
  if /I %XBMC_COMPILE_ANSWER% EQU 2 goto EXE_COMPILE


:XBE_COMPILE
  rem ---------------------------------------------
  rem	check for existing xbe
  rem ---------------------------------------------
  IF EXIST %XBE% (
    goto XBE_EXIST
  )
  goto COMPILE

:EXE_COMPILE
  rem ---------------------------------------------
  rem	check for existing xbe
  rem ---------------------------------------------
  IF EXIST %EXE% (
    goto EXE_EXIST
  )
  goto COMPILE_EXE

:XBE_EXIST
  ECHO ------------------------------------------------------------
  ECHO Found a previous Compiled XBE!
  ECHO [1] a NEW XBE will be compiled for the BUILD 
  ECHO [2] existing XBE will be updated (quick mode compile) for the BUILD
  ECHO [3] existing XBE will be used for the BUILD 
  ECHO ------------------------------------------------------------
  set /P XBMC_COMPILE_ANSWER=Compile a new XBE? [1/2/3]:
  if /I %XBMC_COMPILE_ANSWER% EQU 1 goto COMPILE
  if /I %XBMC_COMPILE_ANSWER% EQU 2 goto COMPILE_NO_CLEAN
  if /I %XBMC_COMPILE_ANSWER% EQU 3 goto MAKE_BUILD

:EXE_EXIST
  ECHO ------------------------------------------------------------
  ECHO Found a previous Compiled WIN32 EXE!
  ECHO [1] a NEW EXE will be compiled for the BUILD_WIN32
  ECHO [2] existing EXE will be updated (quick mode compile) for the BUILD_WIN32
  ECHO [3] existing EXE will be used for the BUILD_WIN32
  ECHO ------------------------------------------------------------
  set /P XBMC_COMPILE_ANSWER=Compile a new EXE? [1/2/3]:
  if /I %XBMC_COMPILE_ANSWER% EQU 1 goto COMPILE_EXE
  if /I %XBMC_COMPILE_ANSWER% EQU 2 goto COMPILE_NO_CLEAN_EXE
  if /I %XBMC_COMPILE_ANSWER% EQU 3 goto MAKE_BUILD_EXE
  
:COMPILE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Cleaning Solution...
  %NET% %CLEAN%
  del release\xbmc.map
  ECHO Compiling Solution...
  %NET% %OPTS%
  IF NOT EXIST %XBE% (
  	set DIETEXT=Default.xbe failed to build!  See .\Release\BuildLog.htm for details.
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO MAKE_BUILD

:COMPILE_EXE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Cleaning Solution...
  %NET% %CLEAN_EXE%
  del  tools\Win32\Release\xbmc.map
  ECHO Compiling Solution...
  %NET% %OPTS_EXE%
  IF NOT EXIST %EXE% (
  	set DIETEXT=XBMC_PC.EXE failed to build!  See .\tools\Win32\Release\BuildLog.htm for details.
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO MAKE_BUILD_EXE
  
:COMPILE_NO_CLEAN
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Compiling Solution...
  %NET% %OPTS%
  IF NOT EXIST %XBE% (
  	set DIETEXT=Default.xbe failed to build!  See .\Release\BuildLog.htm for details.
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO MAKE_BUILD
  
:COMPILE_NO_CLEAN_EXE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Compiling Solution...
  %NET% %OPTS_EXE%
  IF NOT EXIST %EXE% (
  	set DIETEXT=XBMC_PC.EXE failed to build!  See .\tools\Win32\Release\BuildLog.htm for details.
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO :MAKE_BUILD_EXE

:MAKE_BUILD
  ECHO Copying files...
  ECHO - XBE Patching %XBE% 
  %XBE_PATCH% %XBE%
  ECHO - Patching Done!
  
  rmdir BUILD /S /Q
  md BUILD
  
  Echo .svn>exclude.txt
  Echo Thumbs.db>>exclude.txt
  Echo Desktop.ini>>exclude.txt
  Echo dsstdfx.bin>>exclude.txt
  Echo exclude.txt>>exclude.txt

  xcopy %XBE% BUILD
  xcopy UserData BUILD\UserData /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy *.txt BUILD /EXCLUDE:exclude.txt
  rem xcopy *.xml BUILD\

  cd "skin\Project Mayhem III"
  CALL build.bat
  cd ..\..
  xcopy "skin\Project Mayhem III\BUILD\Project Mayhem III" "BUILD\skin\Project Mayhem III" /E /Q /I /Y /EXCLUDE:exclude.txt

  xcopy credits BUILD\credits /Q /I /Y /EXCLUDE:exclude.txt
  xcopy language BUILD\language /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy screensavers BUILD\screensavers /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy visualisations BUILD\visualisations /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy system BUILD\system /E /Q /I /Y /EXCLUDE:exclude.txt
  rem %rar% x web\Project_Mayhem_webserver*.rar build\web\
  rem %rar% x web\Project_Mayem_III_webserver*.rar build\web\
  xcopy media BUILD\media /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy sounds BUILD\sounds /E /Q /I /Y /EXCLUDE:exclude.txt

  del exclude.txt
  ECHO ------------------------------------------------------------
  ECHO Build Succeeded!
  GOTO RAR_XBE

:MAKE_BUILD_EXE
  ECHO Copying files...
  rmdir BUILD_WIN32 /S /Q
  md BUILD_WIN32

  Echo .svn>exclude.txt
  Echo Thumbs.db>>exclude.txt
  Echo Desktop.ini>>exclude.txt
  Echo dsstdfx.bin>>exclude.txt
  Echo exclude.txt>>exclude.txt

  xcopy %EXE% BUILD_WIN32
  xcopy UserData BUILD_WIN32\UserData /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy *.txt BUILD_WIN32 /EXCLUDE:exclude.txt
  rem xcopy *.xml BUILD_WIN32\
  
  rem xcopy tools\Win32\run_me_first.bat BUILD_WIN32 /EXCLUDE:exclude.txt
  Echo subst q: ..>run_me.bat
  Echo subst p: q:\userdata>>run_me.bat
  Echo subst t: q:\userdata>>run_me.bat
  Echo subst z: q:\cache>>run_me.bat
  Echo XBMC_PC.exe>>run_me.bat
  Echo subst q: /D>>run_me.bat
  Echo subst p: /D>>run_me.bat
  Echo subst t: /D>>run_me.bat
  Echo subst z: /D>>run_me.bat
  xcopy run_me.bat BUILD_WIN32
  del run_me.bat
  
  cd "skin\Project Mayhem III"
  CALL build.bat
  cd ..\..
  xcopy "skin\Project Mayhem III\BUILD\Project Mayhem III" "BUILD_WIN32\skin\Project Mayhem III" /E /Q /I /Y /EXCLUDE:exclude.txt

  xcopy credits BUILD_WIN32\credits /Q /I /Y /EXCLUDE:exclude.txt
  xcopy language BUILD_WIN32\language /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy screensavers BUILD_WIN32\screensavers /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy visualisations BUILD_WIN32\visualisations /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy system BUILD_WIN32\system /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy media BUILD_WIN32\media /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy sounds BUILD_WIN32\sounds /E /Q /I /Y /EXCLUDE:exclude.txt

  del exclude.txt
  ECHO ------------------------------------------------------------
  ECHO Build Succeeded!
  GOTO RAR_EXE

:RAR_XBE
  ECHO ------------------------------------------------------------
  ECHO Compressing build to XBMC.rar file...
  ECHO ------------------------------------------------------------
  IF EXIST %RAR% ( %RAR% %RAROPS% 
    ) ELSE ( 
    IF EXIST %RAR_ROOT% ( %RAR_ROOT% %RAROPS% 
      ) ELSE ( 
      ECHO WinRAR not installed!  Skipping .rar compression...
      )
    )
  ECHO ------------------------------------------------------------
  GOTO VIEWLOG_XBE

:RAR_EXE
  ECHO ------------------------------------------------------------
  ECHO Compressing build to XBMC_WIN32.rar file...
  ECHO ------------------------------------------------------------
  IF EXIST %RAR% ( %RAR% %RAROPS_EXE%
    ) ELSE ( 
    IF EXIST %RAR_ROOT% ( %RAR_ROOT% %RAROPS_EXE% 
      ) ELSE (
      ECHO WinRAR not installed!  Skipping .rar compression...
      )
    )
  ECHO ------------------------------------------------------------
  GOTO VIEWLOG_EXE
  
:DIE
  ECHO ------------------------------------------------------------
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  ECHO    ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  set DIETEXT=ERROR: %DIETEXT%
  echo %DIETEXT%
  ECHO ------------------------------------------------------------

:VIEWLOG_XBE
  set /P XBMC_BUILD_ANSWER=View the build log in your HTML browser? [y/n]
  if /I %XBMC_BUILD_ANSWER% NEQ y goto END
  start /D"%~dp0Release" BuildLog.htm"
  goto END

:VIEWLOG_EXE
  set /P XBMC_BUILD_ANSWER=View the build log in your HTML browser? [y/n]
  if /I %XBMC_BUILD_ANSWER% NEQ y goto END
  start /D"%~dp0tools\Win32\Release" BuildLog.htm"
  goto END

:END
  set XBMC_BUILD_ANSWER=
  ECHO Press any key to exit...
  pause > NUL
