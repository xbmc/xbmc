@echo off
ECHO ----------------------------------------
echo Creating estouchy Build Folder
IF Exist ..\..\project\Win32BuildSetup\BUILD_WIN32\addons\skin.estouchy rmdir ..\..\project\Win32BuildSetup\BUILD_WIN32\addons\skin.estouchy /S /Q
md ..\..\project\Win32BuildSetup\BUILD_WIN32\addons\skin.estouchy\media\

Echo build.bat>>exclude.txt
Echo .git>>exclude.txt
Echo \skin.estouchy\media\>>exclude.txt
Echo exclude.txt>>exclude.txt

ECHO ----------------------------------------
ECHO Creating XBT File...
START /B /WAIT ..\..\Tools\TexturePacker\TexturePacker -dupecheck -input media -output ..\..\project\Win32BuildSetup\BUILD_WIN32\addons\skin.estouchy\media\Textures.xbt

ECHO ----------------------------------------
ECHO XBT Texture Files Created...
ECHO Building Skin Directory...
xcopy "..\skin.estouchy" "..\..\project\Win32BuildSetup\BUILD_WIN32\addons\skin.estouchy" /E /Q /I /Y /EXCLUDE:exclude.txt

del exclude.txt
