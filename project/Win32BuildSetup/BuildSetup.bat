@ECHO OFF
rem ----Usage----
rem BuildSetup [vs2008|vs2010] [gl|dx] [clean|noclean]
rem vs2008 for compiling with visual studio 2008 (default)
rem vs2010 for compiling with visual studio 2010
rem gl for opengl build (default)
rem dx for directx build
rem clean to force a full rebuild
rem noclean to force a build without clean
rem noprompt to avoid all prompts
CLS
COLOR 1B
TITLE XBMC for Windows Build Script
rem ----PURPOSE----
rem - Create a working XBMC build with a single click
rem -------------------------------------------------------------
rem Config
rem If you get an error that Visual studio was not found, SET your path for VSNET main executable.
rem -------------------------------------------------------------
rem	CONFIG START
SET comp=vs2008
SET target=dx
SET buildmode=ask
SET promptlevel=prompt
FOR %%b in (%1, %2, %3, %4, %5) DO (
  IF %%b==vs2008 SET comp=vs2008
  IF %%b==vs2010 SET comp=vs2010
	IF %%b==dx SET target=dx
	IF %%b==gl SET target=gl
	IF %%b==clean SET buildmode=clean
	IF %%b==noclean SET buildmode=noclean
	IF %%b==noprompt SET promptlevel=noprompt
)
SET buildconfig=Release (OpenGL)
IF %target%==dx SET buildconfig=Release (DirectX)

IF %comp%==vs2008 (
	IF "%VS90COMNTOOLS%"=="" (
		set NET="%ProgramFiles%\Microsoft Visual Studio 9.0 Express\Common7\IDE\VCExpress.exe"
	) ELSE IF EXIST "%VS90COMNTOOLS%\..\IDE\VCExpress.exe" (
		set NET="%VS90COMNTOOLS%\..\IDE\VCExpress.exe"
	) ELSE IF EXIST "%VS90COMNTOOLS%\..\IDE\devenv.exe" (
		set NET="%VS90COMNTOOLS%\..\IDE\devenv.exe"
	)
) ELSE IF %comp%==vs2010 (
  IF "%VS100COMNTOOLS%"=="" (
		set NET="%ProgramFiles%\Microsoft Visual Studio 10.0\Common7\IDE\VCExpress.exe"
	) ELSE IF EXIST "%VS100COMNTOOLS%\..\IDE\VCExpress.exe" (
		set NET="%VS100COMNTOOLS%\..\IDE\VCExpress.exe"
	) ELSE IF EXIST "%VS100COMNTOOLS%\..\IDE\devenv.exe" (
		set NET="%VS100COMNTOOLS%\..\IDE\devenv.exe"
	)
)

  IF NOT EXIST %NET% (
    IF %comp%==vs2008 (
      set DIETEXT=Visual Studio .NET 2008 Express was not found.
    ELSE IF %comp%==vs2010 (
      set DIETEXT=Visual Studio .NET 2010 Express was not found.
    )
	  goto DIE
  )
  
  IF %comp%==vs2008 (
    set OPTS_EXE="..\VS2008Express\XBMC for Windows.sln" /build "%buildconfig%"
    set CLEAN_EXE="..\VS2008Express\XBMC for Windows.sln" /clean "%buildconfig%"
    set EXE= "..\VS2008Express\XBMC\%buildconfig%\XBMC.exe"
  ) ELSE (
    set OPTS_EXE="..\VS2010Express\XBMC for Windows.sln" /build "%buildconfig%"
    set CLEAN_EXE="..\VS2010Express\XBMC for Windows.sln" /clean "%buildconfig%"
    set EXE= "..\VS2010Express\XBMC\%buildconfig%\XBMC.exe"
  )
	
  rem	CONFIG END
  rem -------------------------------------------------------------

  echo                         :                                                  
  echo                        :::                                                 
  echo                        ::::                                                
  echo                        ::::                                                
  echo    :::::::       :::::::::::::::::        ::::::      ::::::        :::::::
  echo    :::::::::   ::::::::::::::::::::     ::::::::::  ::::::::::    :::::::::
  echo     ::::::::: ::::::::::::::::::::::   ::::::::::::::::::::::::  ::::::::: 
  echo          :::::::::     :::      ::::: :::::    ::::::::    :::: :::::      
  echo           ::::::      ::::       :::: ::::      :::::       :::::::        
  echo           :::::       ::::        :::::::       :::::       ::::::         
  echo           :::::       :::         ::::::         :::        ::::::         
  echo           ::::        :::         ::::::        ::::        ::::::         
  echo           ::::        :::        :::::::        ::::        ::::::         
  echo          :::::        ::::       :::::::        ::::        ::::::         
  echo         :::::::       ::::      ::::::::        :::         :::::::        
  echo     :::::::::::::::    :::::  ::::: :::         :::         :::::::::      
  echo  :::::::::  :::::::::  :::::::::::  :::         :::         ::: :::::::::  
  echo  ::::::::    :::::::::  :::::::::   :::         :::         :::  ::::::::  
  echo ::::::         :::::::    :::::     :            ::          ::    ::::::  
  goto EXE_COMPILE

:EXE_COMPILE
  IF %buildmode%==clean goto COMPILE_EXE
  rem ---------------------------------------------
  rem	check for existing exe
  rem ---------------------------------------------
  IF EXIST buildlog.html del buildlog.html /q
  
  IF EXIST %EXE% (
    goto EXE_EXIST
  )
  goto COMPILE_EXE

:EXE_EXIST
  IF %buildmode%==noclean goto COMPILE_NO_CLEAN_EXE
  ECHO ------------------------------------------------------------
  ECHO Found a previous Compiled WIN32 EXE!
  ECHO [1] a NEW EXE will be compiled for the BUILD_WIN32
  ECHO [2] existing EXE will be updated (quick mode compile) for the BUILD_WIN32
  ECHO ------------------------------------------------------------
  set /P XBMC_COMPILE_ANSWER=Compile a new EXE? [1/2]:
  if /I %XBMC_COMPILE_ANSWER% EQU 1 goto COMPILE_EXE
  if /I %XBMC_COMPILE_ANSWER% EQU 2 goto COMPILE_NO_CLEAN_EXE
  
:COMPILE_EXE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Cleaning Solution...
  %NET% %CLEAN_EXE%
  ECHO Compiling XBMC...
  %NET% %OPTS_EXE%
  IF NOT EXIST %EXE% (
  	set DIETEXT="XBMC.EXE failed to build!  See ..\vs2008express\XBMC\%buildconfig%\BuildLog.htm for details."
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO MAKE_BUILD_EXE
  
:COMPILE_NO_CLEAN_EXE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Compiling Solution...
  %NET% %OPTS_EXE%
  IF NOT EXIST %EXE% (
  	set DIETEXT="XBMC.EXE failed to build!  See ..\vs2008express\XBMC\%buildconfig%\BuildLog\BuildLog.htm for details."
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO MAKE_BUILD_EXE

:MAKE_BUILD_EXE
  ECHO Copying files...
  IF EXIST BUILD_WIN32 rmdir BUILD_WIN32 /S /Q

  Echo .svn>exclude.txt
  Echo CVS>>exclude.txt
  Echo .so>>exclude.txt
  Echo Thumbs.db>>exclude.txt
  Echo Desktop.ini>>exclude.txt
  Echo dsstdfx.bin>>exclude.txt
  Echo exclude.txt>>exclude.txt
  rem and exclude potential leftovers
  Echo mediasources.xml>>exclude.txt
  Echo advancedsettings.xml>>exclude.txt
  Echo guisettings.xml>>exclude.txt
  Echo profiles.xml>>exclude.txt
  Echo sources.xml>>exclude.txt
  Echo userdata\cache\>>exclude.txt
  Echo userdata\database\>>exclude.txt
  Echo userdata\playlists\>>exclude.txt
  Echo userdata\script_data\>>exclude.txt
  Echo userdata\thumbnails\>>exclude.txt
  rem UserData\visualisations contains currently only xbox visualisationfiles
  Echo userdata\visualisations\>>exclude.txt
  rem other platform stuff
  Echo lib-osx>>exclude.txt
  Echo players\mplayer>>exclude.txt
  Echo FileZilla Server.xml>>exclude.txt
  Echo asound.conf>>exclude.txt
  Echo voicemasks.xml>>exclude.txt
  Echo Lircmap.xml>>exclude.txt
  
  md BUILD_WIN32\Xbmc

  xcopy %EXE% BUILD_WIN32\Xbmc > NUL
  xcopy ..\..\userdata BUILD_WIN32\Xbmc\userdata /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
  copy ..\..\copying.txt BUILD_WIN32\Xbmc > NUL
  copy ..\..\LICENSE.GPL BUILD_WIN32\Xbmc > NUL
  copy ..\..\known_issues.txt BUILD_WIN32\Xbmc > NUL
  xcopy dependencies\*.* BUILD_WIN32\Xbmc /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  IF %comp%==vs2008 (
    xcopy vs_redistributable\vs2008\vcredist_x86.exe BUILD_WIN32\Xbmc /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  ) ELSE (
    xcopy vs_redistributable\vs2010\vcredist_x86.exe BUILD_WIN32\Xbmc /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  )
  copy sources.xml BUILD_WIN32\Xbmc\userdata > NUL
  
  xcopy ..\..\language BUILD_WIN32\Xbmc\language /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  xcopy ..\..\addons BUILD_WIN32\Xbmc\addons /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
  xcopy ..\..\system BUILD_WIN32\Xbmc\system /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  xcopy ..\..\media BUILD_WIN32\Xbmc\media /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  xcopy ..\..\sounds BUILD_WIN32\Xbmc\sounds /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
    
  IF EXIST error.log del error.log > NUL
  SET build_path=%CD%
  ECHO ------------------------------------------------------------
  ECHO Building Confluence Skin...
  cd ..\..\addons\skin.confluence
  call build.bat > NUL
  cd %build_path%
  rem restore color and title, some scripts mess these up
  COLOR 1B
  TITLE XBMC for Windows Build Script

  IF EXIST exclude.txt del exclude.txt  > NUL
  ECHO ------------------------------------------------------------
  ECHO Build Succeeded!
  GOTO NSIS_EXE

:NSIS_EXE
  ECHO ------------------------------------------------------------
  ECHO Generating installer includes...
  call genNsisIncludes.bat
  ECHO ------------------------------------------------------------
  CALL extract_git_rev.bat
  SET GIT_REV=#%GIT_REV%
  SET XBMC_SETUPFILE=XBMCSetup-Rev%GIT_REV%-%target%.exe
  ECHO Creating installer %XBMC_SETUPFILE%...
  IF EXIST %XBMC_SETUPFILE% del %XBMC_SETUPFILE% > NUL
  rem get path to makensis.exe from registry, first try tab delim
  FOR /F "tokens=2* delims=	" %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B

  IF NOT EXIST "%NSISExePath%" (
    rem try with space delim instead of tab
    FOR /F "tokens=2* delims= " %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
  )
      
  IF NOT EXIST "%NSISExePath%" (
    rem fails on localized windows (Default) becomes (Par Défaut)
    FOR /F "tokens=3* delims=	" %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
  )

  IF NOT EXIST "%NSISExePath%" (
    FOR /F "tokens=3* delims= " %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
  )
  
  rem proper x64 registry checks
  IF NOT EXIST "%NSISExePath%" (
    ECHO using x64 registry entries
    FOR /F "tokens=2* delims=	" %%A IN ('REG QUERY "HKLM\Software\Wow6432Node\NSIS" /ve') DO SET NSISExePath=%%B
  )
  IF NOT EXIST "%NSISExePath%" (
    rem try with space delim instead of tab
    FOR /F "tokens=2* delims= " %%A IN ('REG QUERY "HKLM\Software\Wow6432Node\NSIS" /ve') DO SET NSISExePath=%%B
  )
  IF NOT EXIST "%NSISExePath%" (
    rem on win 7 x64, the previous fails
    FOR /F "tokens=3* delims=	" %%A IN ('REG QUERY "HKLM\Software\Wow6432Node\NSIS" /ve') DO SET NSISExePath=%%B
  )
  IF NOT EXIST "%NSISExePath%" (
    rem try with space delim instead of tab
    FOR /F "tokens=3* delims= " %%A IN ('REG QUERY "HKLM\Software\Wow6432Node\NSIS" /ve') DO SET NSISExePath=%%B
  )

  SET NSISExe=%NSISExePath%\makensis.exe
  "%NSISExe%" /V1 /X"SetCompressor /FINAL lzma" /Dxbmc_root="%CD%\BUILD_WIN32" /Dxbmc_revision="%GIT_REV%" /Dxbmc_target="%target%" "XBMC for Windows.nsi"
  IF NOT EXIST "%XBMC_SETUPFILE%" (
	  set DIETEXT=Failed to create %XBMC_SETUPFILE%.
	  goto DIE
  )
  ECHO ------------------------------------------------------------
  ECHO Done!
  ECHO Setup is located at %CD%\%XBMC_SETUPFILE%
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

:VIEWLOG_EXE
  IF %promptlevel%==noprompt (
  goto END
  )
  IF %comp%==vs2008 (
    SET log="%CD%\..\vs2008express\XBMC\%buildconfig%\objs\BuildLog.htm"
  ) ELSE (
    SET log="%CD%\..\vs2010express\XBMC\%buildconfig%\objs\BuildLog.htm"
  )
  IF NOT EXIST %log% goto END
  
  copy %log% ./buildlog.html > NUL
  
  set /P XBMC_BUILD_ANSWER=View the build log in your HTML browser? [y/n]
  if /I %XBMC_BUILD_ANSWER% NEQ y goto END
  
  IF %comp%==vs2008 (
    SET log="%CD%\..\vs2008express\XBMC\%buildconfig%\objs\" BuildLog.htm
  ) ELSE (
    SET log="%CD%\..\vs2010express\XBMC\%buildconfig%\objs\" BuildLog.htm
  )
  
  start /D%log%
  goto END

:END
  IF %promptlevel% NEQ noprompt (
  ECHO Press any key to exit...
  pause > NUL
