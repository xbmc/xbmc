@ECHO OFF
@setlocal 

rem
rem BuildSetup.sh (v1.1)
rem
rem PURPOSE - BuildSetup creates a complete XBMC installer package 
rem
rem Usage:
rem   BuildSetup [options] - Default options: dx noclean vs2010 errpromp mingwlibs
rem
rem Option	Explanation
rem ---------   -----------------------------------------------
rem dx		Build for DirectX (default)
rem noclean	Build without clean (default)
rem vs2010	Compile with Visual Studio 2010 (default)
rem errprompt	Only pause or prompt on error (default)
rem mingwlibs	Build or rebuil mingwlibs only when neccesary (default) 
rem
rem gl		Build for OpenGL (not supported)
rem clean	Force a full rebuild i.e. rebuilds everything
rem prompt	Always prompt
rem noprompt	Avoid all prompts
rem nomingwlibs	Skip nomingwlibs 


CLS
COLOR 1B
TITLE XBMC for Windows Build Script

::
:: Set default options
::

SET comp=vs2010
SET graphics=dx
SET buildmode=noclean
SET promptlevel=errprompt
SET buildmingwlibs=true
SET exitcode=0

::
:: Check command line options
:: 
FOR %%b in (%*) DO (
	IF %%b==vs2010 SET comp=vs2010
	IF %%b==dx SET videmode=dx
	IF %%b==gl echo "%0%: option gl currently not supported" 
	IF %%b==clean SET buildmode=clean
	IF %%b==noprompt SET promptlevel=noprompt
	IF %%b==prompt SET promptlevel=prompt
	IF %%b==nomingwlibs SET buildmingwlibs=false
)

::
:: Clean up section
::
	
IF EXIST buildlog.html del buildlog.html /q
IF EXIST errormingw del errormingw /q 
IF EXIST prompt del prompt /q 
IF EXIST makeclean del makeclean /q 

::
:: Check for Visual Studio and select solution target 
::

IF %comp%==vs2010 (
  IF "%VS100COMNTOOLS%"=="" (
	set VS_EXE="%ProgramFiles%\Microsoft Visual Studio 10.0\Common7\IDE\VCExpress.exe"
  ) ELSE IF EXIST  "%VS100COMNTOOLS%..\IDE\devenv.exe" (
	SET VS_EXE="%VS100COMNTOOLS%..\IDE\devenv.exe"
  ) ELSE IF EXIST  "%VS100COMNTOOLS%..\IDE\VCExpress.exe" (
	SET VS_EXE="%VS100COMNTOOLS%..\IDE\VCExpress.exe"
  )

  IF NOT EXIST %VS_EXE% (
	SET DIETEXT="Visual Studio 2010 Express not installed"
	goto DIE
  )
) ELSE (
	SET DIETEXT="Unsupported Compiler: %comp%"
	GOTO DIE
)

::
:: Check for existing xbcmc.exe 
:: 

IF %graphics%==dx SET RELEASE=Release (DirectX)
IF %graphics%==gl SET RELEASE=Release (OpenGL)

set XBMC_EXE="..\VS2010Express\XBMC\%RELEASE%\XBMC.exe"
set XBMC_PDB="..\VS2010Express\XBMC\%RELEASE%\XBMC.pdb"

IF EXIST %XBMC_EXE% IF "%promptlevel%"=="prompt" (
	ECHO ------------------------------------------------------------
	ECHO Found a previous Compiled XBMC.EXE
	ECHO 1. Rebuild changed files only. [default]
	ECHO 2. Force rebuild everything.
	ECHO ------------------------------------------------------------
	SET ANSWER=1 
	set /P ANSWER="Compile mode: 1/2? [1]:"

	if /I "%ANSWER%" EQU 2 (
		set buildmode=clean
	) else (
		set buildmode=noclean
	)
)


::
:: Build MinGWlibs
::

IF %buildmingwlibs%==true (
 	IF %buildmode%==clean (
		echo Force rebuild of MinGWlibs
	) else IF EXIST mingwlibsok (
		:: echo MinGWlibs already exists [skipping...]
		SET buildmingwlibs=false
	)
)

IF %buildmingwlibs%==true (
  ECHO ------------------------------------------------------------
  ECHO Compiling MinGWlibs libraries...

  IF %buildmode%==clean ECHO makeclean>makeclean
  IF %promptlevel%==prompt ECHO prompt>prompt

  call buildmingwlibs.bat

  IF NOT EXIST mingwlibsok (
  	set DIETEXT="failed to build mingw libs"
  	goto DIE
  )

  IF EXIST errormingw (
  	set DIETEXT="failed to build mingw libs"
  	goto DIE
  )
)

::
:: Build the XBMC solution with Visual Studio command line
::

IF %buildmode%==clean (
	set BUILDFLAG=/rebuild
) else (
	set BUILDFLAG=/build
)

ECHO ------------------------------------------------------------
ECHO Compiling XBMC %BUILDFLAG% %RELEASE% ...

%VS_EXE% "..\VS2010Express\XBMC for Windows.sln" %BUILDFLAG% "%RELEASE%"

::
:: Check that target exists
::

IF NOT EXIST %XBMC_EXE% (
	set DIETEXT="XBMC.EXE failed to build!  See %CD%\..\vs2010express\XBMC\%RELEASE%\objs\XBMC.log"
	IF %promptlevel%==noprompt (
		type "%CD%\..\vs2010express\XBMC\%RELEASE%\objs\XBMC.log"
	)
	goto DIE
)

::
:: Copy installer package files
::

ECHO ------------------------------------------------------------
ECHO Copying files...
(
	echo .svn
	echo CVS
	echo Thumbs.db
	echo Desktop.ini
	echo dsstdfx.bin
	echo exclude.txt
 	:: and exclude potential leftovers
	echo mediasources.xml
	echo advancedsettings.xml
	echo guisettings.xml
	echo profiles.xml
	echo sources.xml
	echo userdata\cache\
	echo userdata\database\
	echo userdata\playlists\
	echo userdata\script_data\
	echo userdata\thumbnails\
	:: UserData\visualisations contains currently only xbox visualisationfiles
	echo userdata\visualisations\
	:: other platform stuff
	echo lib-osx
	echo players\mplayer
	echo FileZilla Server.xml
	echo asound.conf
 	echo voicemasks.xml
	echo Lircmap.xml

) > exclude.txt

IF EXIST BUILD_WIN32 rmdir BUILD_WIN32 /S /Q
md BUILD_WIN32\Xbmc

xcopy %XBMC_EXE% BUILD_WIN32\Xbmc > NUL
xcopy ..\..\userdata BUILD_WIN32\Xbmc\userdata /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
copy ..\..\copying.txt BUILD_WIN32\Xbmc > NUL
copy ..\..\LICENSE.GPL BUILD_WIN32\Xbmc > NUL
copy ..\..\known_issues.txt BUILD_WIN32\Xbmc > NUL
xcopy dependencies\*.* BUILD_WIN32\Xbmc /Q /I /Y /EXCLUDE:exclude.txt  > NUL
copy sources.xml BUILD_WIN32\Xbmc\userdata > NUL

xcopy ..\..\language BUILD_WIN32\Xbmc\language /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
xcopy ..\..\addons BUILD_WIN32\Xbmc\addons /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
xcopy ..\..\system BUILD_WIN32\Xbmc\system /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
xcopy ..\..\media BUILD_WIN32\Xbmc\media /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
xcopy ..\..\sounds BUILD_WIN32\Xbmc\sounds /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL

ECHO ------------------------------------------------------------
ECHO Building PVR add ons...
call buildpvraddons.bat %NET%
  
IF EXIST error.log del /q error.log 2> NUL
SET build_path=%CD%
ECHO ------------------------------------------------------------
ECHO Building Confluence Skin...
cd ..\..\addons\skin.confluence
call build.bat 2>&1 > NUL  
cd %build_path%
:: restore color and title, some scripts mess these up
COLOR 1B
TITLE XBMC for Windows Build Script

IF EXIST exclude.txt del exclude.txt  2> NUL
del /s /q /f BUILD_WIN32\Xbmc\*.so    2> NUL
del /s /q /f BUILD_WIN32\Xbmc\*.h     2> NUL
del /s /q /f BUILD_WIN32\Xbmc\*.cpp   2> NUL
del /s /q /f BUILD_WIN32\Xbmc\*.exp   2> NUL
del /s /q /f BUILD_WIN32\Xbmc\*.lib   2> NUL

ECHO ------------------------------------------------------------
ECHO Creating installer includes, deploy dependencies and git info
call genNsisIncludes.bat
call getdeploydependencies.bat
call extract_git_rev.bat > NUL
ECHO ------------------------------------------------------------
SET XBMC_SETUPFILE=XBMCSetup-%GIT_REV%-%graphics%.exe
SET XBMC_PDBFILE=XBMCSetup-%GIT_REV%-%target%.pdb
ECHO Generating XBMC package installer: %XBMC_SETUPFILE%...


IF EXIST %XBMC_SETUPFILE% del %XBMC_SETUPFILE% > NUL
:: get path to makensis.exe from registry, first try tab delim
FOR /F "tokens=2* delims=	" %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B

IF NOT EXIST "%NSISExePath%" (
  :: try with space delim instead of tab
  FOR /F "tokens=2* delims= " %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
)
    
IF NOT EXIST "%NSISExePath%" (
  :: fails on localized windows (Default) becomes (Par D�faut)
  FOR /F "tokens=3* delims=	" %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
)

IF NOT EXIST "%NSISExePath%" (
  FOR /F "tokens=3* delims= " %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
)

:: proper x64 registry checks
IF NOT EXIST "%NSISExePath%" (
  ECHO using x64 registry entries
  FOR /F "tokens=2* delims=	" %%A IN ('REG QUERY "HKLM\Software\Wow6432Node\NSIS" /ve') DO SET NSISExePath=%%B
)
IF NOT EXIST "%NSISExePath%" (
  :: try with space delim instead of tab
  FOR /F "tokens=2* delims= " %%A IN ('REG QUERY "HKLM\Software\Wow6432Node\NSIS" /ve') DO SET NSISExePath=%%B
)
IF NOT EXIST "%NSISExePath%" (
  :: on win 7 x64, the previous fails
  FOR /F "tokens=3* delims=	" %%A IN ('REG QUERY "HKLM\Software\Wow6432Node\NSIS" /ve') DO SET NSISExePath=%%B
)
IF NOT EXIST "%NSISExePath%" (
  :: try with space delim instead of tab
  FOR /F "tokens=3* delims= " %%A IN ('REG QUERY "HKLM\Software\Wow6432Node\NSIS" /ve') DO SET NSISExePath=%%B
)

SET NSISExe=%NSISExePath%\makensis.exe
"%NSISExe%" /V1 /X"SetCompressor /FINAL lzma" /Dxbmc_root="%CD%\BUILD_WIN32" /Dxbmc_revision="%GIT_REV%" /Dxbmc_target="%graphics%" "XBMC for Windows.nsi"
IF NOT EXIST "%XBMC_SETUPFILE%" (
	  set DIETEXT=Failed to create %XBMC_SETUPFILE%. NSIS installed?
	  goto DIE
)

:: pdb file needed by Jenkins
copy %XBMC_PDB% %XBMC_PDBFILE% > nul

ECHO Done!
ECHO Setup is located at: %CD%\%XBMC_SETUPFILE%
ECHO ------------------------------------------------------------
GOTO VIEWLOG_EXE
  
:DIE
  ECHO ------------------------------------------------------------
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  ECHO    ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  set DIETEXT=ERROR: %DIETEXT%
  echo %DIETEXT%
  SET exitcode=1
  ECHO ------------------------------------------------------------

:VIEWLOG_EXE
  SET log="%CD%\..\vs2010express\XBMC\%RELEASE%\objs\XBMC.log"

  IF EXIST %log% (
	echo "Log: %log%"

  	IF "%promptlevel%"=="prompt" (

	  set /P XBMC_BUILD_ANSWER=View the build log in your HTML browser? y/n? [n]:
  	  if /I "%XBMC_BUILD_ANSWER%"=="y" (
		  copy %log% buildlog.html > NUL
		  set log=buildlog.html 

  	  start /D%log%
  	)
  )
 

:END
  IF "%promptlevel%"=="prompt" (
  	ECHO Press any key to exit...
  	pause > NUL
  )
  ENDLOCAL
  EXIT /B %exitcode%
