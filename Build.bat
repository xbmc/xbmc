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
rem ---------------------------------------------
TITLE XBMC Build Prepare Script
ECHO Wait while preparing the build.
ECHO ------------------------------
rem	CONFIG START
	set NET=C:\Progra~1\Micros~1.NET\Common7\IDE\devenv.exe
	set OPTS=xbmc.sln /build release
	set CLEAN=xbmc.sln /clean release
	set XBE=xbepatch.exe
	set RAR=C:\Progra~1\Winrar\rar.exe
	set RAROPS=a -r -idp -inul -m5 XBMC.rar BUILD
	set TEX=XBMCTex.exe
	set SKINS=..\Skins
	set CODECS=..\Codecs
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
xcopy mplayer BUILD\mplayer /E /Q /I /Y
xcopy skin\MediaCenter\fonts BUILD\skin\MediaCenter\fonts /E /Q /I /Y
xcopy skin\*.xml BUILD\skin /E /Q /I /Y
%TEX% -input BUILD\skin\MediaCenter\media -output BUILD\skin\MediaCenter\media
xcopy language BUILD\language /E /Q /I /Y
xcopy xbmc\keyboard\media BUILD\media /E /Q /I /Y
xcopy visualisations BUILD\visualisations /E /Q /I /Y
xcopy weather BUILD\weather /E /Q /I /Y
rem xcopy web BUILD\web /E /Q /I /Y
rem xcopy python BUILD\python /E /Q /I /Y
rem xcopy %SKINS% Build\Skin /E /Q /I /Y
rem xcopy %CODECS% Build\mplayer\codecs /E /Q /I /Y
ECHO ------------------------------
ECHO Rarring...
%RAR% %RAROPS%
ECHO ------------------------------
ECHO finished!