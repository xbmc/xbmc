@ECHO OFF

rem ----PURPOSE----
rem - Create a working XBMC build with a single click
rem ---------------------------------------------
rem Config
rem Set your path for VSNET main executable
rem Set the path for xbepatch
rem Set the path for WinRARs rar.exe (freeware)
rem and finally set the options for the final rar.
rem ---------------------------------------------
rem Remove 'rem' from %NET% to compile and/or clean the solution prior packing it.
rem Remove 'rem' from 'xcopy web/python' to copy these to the BUILD directory.
rem pike: I've temporarily commented out xbmctex sections. Let's call it an evaluation.. 
rem ---------------------------------------------
TITLE XBMC Build Prepare Script
ECHO Wait while preparing the build.
ECHO ------------------------------
rem	CONFIG START
	set NET=C:\Progra~1\Micros~1.NET\Common7\IDE\devenv.com
	set OPTS=xbmc.sln /build release
	set CLEAN=xbmc.sln /clean release
	set XBE=xbepatch.exe
	set RAR=C:\Progra~1\Winrar\rar.exe
	set RAROPS=a -r -idp -inul -m5 XBMC.rar BUILD
	set TEX=XBMCTex.exe
	set SKINS=..\Skins
rem	CONFIG END
rem ---------------------------------------------
ECHO Compiling Solution...
%NET% %CLEAN%
del release\xbmc.map
%NET% %OPTS%
ECHO Done!
ECHO ------------------------------
ECHO Copying files...
%XBE% release\default.xbe
rmdir BUILD /S /Q
md BUILD
copy release\default.xbe BUILD
copy *.xml BUILD
copy *.txt BUILD
xcopy skin\Projec~1\fonts "BUILD\skin\Project Mayhem\fonts" /E /Q /I /Y
xcopy skin\Projec~1\*.xml "BUILD\skin\Project Mayhem\" /E /Q /I /Y
REM %TEX% -input skin\Projec~1\media -output "BUILD\skin\Project Mayhem\media"
xcopy skin\Projec~1\media\Textures.xpr "BUILD\skin\Project Mayhem\media" /Q /I /Y
xcopy skin\Projec~1\media\background.png "BUILD\skin\Project Mayhem\media" /Q /I /Y
xcopy skin\Projec~1\media\background2.png "BUILD\skin\Project Mayhem\media" /Q /I /Y
xcopy credits BUILD\credits /Q /I /Y
xcopy language BUILD\language /E /Q /I /Y
xcopy xbmc\keyboard\media BUILD\media /E /Q /I /Y
xcopy screensavers BUILD\screensavers /E /Q /I /Y
xcopy visualisations BUILD\visualisations /E /Q /I /Y
xcopy weather BUILD\weather /E /Q /I /Y
xcopy system BUILD\system /E /Q /I /Y
rem xcopy web BUILD\web /E /Q /I /Y
rem xcopy python BUILD\python /E /Q /I /Y
rem xcopy %SKINS% Build\Skin /E /Q /I /Y
xcopy media BUILD\media /E /Q /I /Y
del BUILD\media\dsstdfx.bin
del BUILD\system\players\mplayer\codecs\.cvsignore

REM ECHO ------------------------------
REM ECHO Removing CVS directories from build
REM FOR /R BUILD %%d IN (CVS) DO @RD /S /Q %%d

ECHO ------------------------------
ECHO Rarring...
%RAR% %RAROPS%

ECHO ------------------------------
ECHO finished!