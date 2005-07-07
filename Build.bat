@ECHO OFF

rem ----PURPOSE----
rem - Create a working XBMC build with a single click
rem ---------------------------------------------
rem Config
rem Set your path for VSNET main executable
rem Set the path for xbepatch (this is only necessary if you have a very old bios)
rem Set the path for WinRAR's rar.exe (freeware)
rem and finally set the options for the final rar.
rem ---------------------------------------------
rem Remove 'rem' from %NET% to compile and/or clean the solution prior packing it.
rem Remove 'rem' from 'xcopy web/python' to copy these to the BUILD directory.
rem ---------------------------------------------
TITLE XBMC Build Prepare Script
ECHO Wait while preparing the build.
ECHO ------------------------------
rem	CONFIG START
	set NET="C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv.com"
	set OPTS=xbmc.sln /build release
	set CLEAN=xbmc.sln /clean release
	set XBE=xbepatch.exe
	set RAR="C:\Program Files\Winrar\rar.exe"
	set RAROPS1=a -r -idp -inul -m5 XBMC.rar BUILD
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
xcopy "skin\Project Mayhem III\fonts" "BUILD\skin\Project Mayhem III\fonts" /E /Q /I /Y
xcopy "skin\Project Mayhem III\*.xml" "BUILD\skin\Project Mayhem III\" /E /Q /I /Y
xcopy "skin\Project Mayhem III\media\Textures.xpr" "BUILD\skin\Project Mayhem III\media" /Q /I /Y
xcopy "skin\Project Mayhem III\sounds\*.*" "BUILD\skin\Project Mayhem III\sounds" /Q /I /Y 
xcopy credits BUILD\credits /Q /I /Y
xcopy language BUILD\language /E /Q /I /Y
xcopy screensavers BUILD\screensavers /E /Q /I /Y
xcopy visualisations BUILD\visualisations /E /Q /I /Y
xcopy system BUILD\system /E /Q /I /Y
rem %rar% x web\Project_Mayhem_webserver*.rar build\web\
rem xcopy python BUILD\python /E /Q /I /Y
rem xcopy %SKINS% Build\Skin /E /Q /I /Y
xcopy media BUILD\media /E /Q /I /Y
xcopy sounds BUILD\sounds /E /Q /I /Y
del BUILD\media\dsstdfx.bin
del BUILD\system\players\mplayer\codecs\.cvsignore

ECHO ------------------------------
ECHO Removing CVS directories from build
FOR /R BUILD %%d IN (CVS) DO @RD /S /Q %%d

ECHO ------------------------------
ECHO Rarring...
%RAR% %RAROPS1%

ECHO ------------------------------
ECHO finished!