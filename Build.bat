@echo off
ECHO ----------------------------------------
echo Creating MediaStream Build Folder
rmdir ..\..\project\Win32BuildSetup\BUILD_WIN32\Xbmc\addons\skin.mediastream\ /S /Q
md ..\..\project\Win32BuildSetup\BUILD_WIN32\Xbmc\addons\skin.mediastream\media\

Echo .svn>exclude.txt
Echo Thumbs.db>>exclude.txt
Echo Desktop.ini>>exclude.txt
Echo dsstdfx.bin>>exclude.txt
Echo BUILD>>exclude.txt
Echo \skin.mediastream\media\>>exclude.txt
Echo exclude.txt>>exclude.txt

ECHO ----------------------------------------
ECHO Creating XBT File...
START /B /WAIT ..\..\Tools\TexturePacker\TexturePacker -input media -output ..\..\project\Win32BuildSetup\BUILD_WIN32\Xbmc\addons\skin.mediastream\media\Textures.xbt

ECHO ----------------------------------------
ECHO XBT Texture Files Created...
ECHO Building Skin Directory...
xcopy "..\skin.mediastream" "..\..\project\Win32BuildSetup\BUILD_WIN32\Xbmc\addons\skin.mediastream" /E /Q /I /Y /EXCLUDE:exclude.txt

del exclude.txt
