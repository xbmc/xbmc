@echo off
ECHO ----------------------------------------
echo Creating estouchy Build Folder
IF Exist %base_dir%\project\Win32BuildSetup\BUILD_WIN32\addons\skin.estouchy rmdir %base_dir%\project\Win32BuildSetup\BUILD_WIN32\addons\skin.estouchy /S /Q
md %base_dir%\project\Win32BuildSetup\BUILD_WIN32\addons\skin.estouchy\media\

Echo build.bat>>exclude.txt
Echo .git>>exclude.txt
Echo \skin.estouchy\media\>>exclude.txt
Echo exclude.txt>>exclude.txt

ECHO ----------------------------------------
ECHO Creating XBT File...
START /B /WAIT %base_dir%\Tools\TexturePacker\TexturePacker -dupecheck -input media -output %base_dir%\project\Win32BuildSetup\BUILD_WIN32\addons\skin.estouchy\media\Textures.xbt

ECHO ----------------------------------------
ECHO XBT Texture Files Created...
ECHO Building Skin Directory...
xcopy "..\skin.estouchy" "%base_dir%\project\Win32BuildSetup\BUILD_WIN32\addons\skin.estouchy" /E /Q /I /Y /EXCLUDE:exclude.txt

del exclude.txt
