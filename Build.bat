@ECHO OFF

rem ----PURPOSE----
rem - Create a working XBMC build with a single click
rem ---------------------------------------------
rem Config
rem If you get an error that Visual studio was not found, SET your path for VSNET main executable.
rem ONLY needed if you have a very old bios, SET the path for xbepatch. Not needed otherwise.
rem If Winrar isn't installed under standard programs, SET the path for WinRAR's (freeware) rar.exe
rem and finally set the options for the final rar.
rem ---------------------------------------------
rem Remove 'rem' from 'web / python' below to copy these to the BUILD directory.
rem ---------------------------------------------
TITLE XBMC Build Prepare Script
ECHO Wait while preparing the build.
ECHO ------------------------------
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
	set XBE=xbepatch.exe
	set RAR="%ProgramFiles%\Winrar\rar.exe"
	set RAROPS=a -r -idp -inul -m5 XBMC.rar BUILD
rem	CONFIG END
rem ---------------------------------------------
ECHO Compiling Solution...
%NET% %CLEAN%
del release\xbmc.map
%NET% %OPTS%
IF NOT EXIST release\default.xbe (
	set DIETEXT=Default.xbe failed to build!  See .\Release\BuildLog.htm for details.
	goto DIE
) 
ECHO Done!
ECHO ------------------------------
ECHO Copying files...
%XBE% release\default.xbe
rmdir BUILD /S /Q
md BUILD
copy release\default.xbe BUILD
copy *.xml BUILD\
copy *.txt BUILD\
xcopy "skin\Project Mayhem III\fonts" "BUILD\skin\Project Mayhem III\fonts" /E /Q /I /Y
xcopy "skin\Project Mayhem III\*.xml" "BUILD\skin\Project Mayhem III\" /E /Q /I /Y
xcopy "skin\Project Mayhem III\media\Textures.xpr" "BUILD\skin\Project Mayhem III\media\" /Q /I /Y
xcopy "skin\Project Mayhem III\sounds\*.*" "BUILD\skin\Project Mayhem III\sounds\" /Q /I /Y 
xcopy credits BUILD\credits /Q /I /Y
xcopy language BUILD\language /E /Q /I /Y
xcopy screensavers BUILD\screensavers /E /Q /I /Y
xcopy visualisations BUILD\visualisations /E /Q /I /Y
xcopy system BUILD\system /E /Q /I /Y
rem %rar% x web\Project_Mayhem_webserver*.rar build\web\
rem xcopy python BUILD\python /E /Q /I /Y
xcopy media BUILD\media /E /Q /I /Y
xcopy sounds BUILD\sounds /E /Q /I /Y
del BUILD\media\dsstdfx.bin
del BUILD\system\players\mplayer\codecs\.cvsignore 

ECHO ------------------------------
ECHO Removing CVS directories from build
FOR /R BUILD %%d IN (CVS) DO @RD /S /Q "%%d" 2>NUL

ECHO ------------------------------
IF NOT EXIST %RAR% (
	ECHO WinRAR not installed!  Skipping .rar compression...
) ELSE (
	ECHO Compressing build to XBMC.rar file...
	%RAR% %RAROPS%
)

	ECHO ------------------------------
ECHO Build Succeeded! 
GOTO VIEWLOG

:DIE
	ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
set DIETEXT=ERROR: %DIETEXT%
echo %DIETEXT%

:VIEWLOG
REM use a DOS trick to get a y/n user prompt without choice.exe
REM Credit goes to http://www.robvanderwoude.com/amb_userinput_yn.html
type nul>"%temp%\~Yes~No~.tmp"
ECHO View the build log in your HTML browser? [y/n]
del /p "%temp%\~Yes~No~.tmp">nul
if exist "%temp%\~Yes~No~.tmp" goto END
del "%temp%\~Yes~No~.tmp" > NUL: 2>&1
start %~dps0\Release\BuildLog.htm
goto END

:END
ECHO Press any key to exit...
pause > NUL: