@echo off
ECHO ----------------------------------------
echo Creating Estuary Build Folder
IF Exist ..\..\project\Win32BuildSetup\BUILD_WIN32\application\addons\skin.estuary rmdir ..\..\project\Win32BuildSetup\BUILD_WIN32\application\addons\skin.estuary /S /Q
md ..\..\project\Win32BuildSetup\BUILD_WIN32\application\addons\skin.estuary\media\

Echo .svn>exclude.txt
Echo Thumbs.db>>exclude.txt
Echo Desktop.ini>>exclude.txt
Echo dsstdfx.bin>>exclude.txt
Echo build.bat>>exclude.txt
Echo \skin.estuary\media\>>exclude.txt
Echo exclude.txt>>exclude.txt

ECHO ----------------------------------------
ECHO Creating XBT File...
START /B /WAIT ..\..\Tools\TexturePacker\TexturePacker -dupecheck -input media -output ..\..\project\Win32BuildSetup\BUILD_WIN32\application\addons\skin.estuary\media\Textures.xbt
START /B /WAIT ..\..\Tools\TexturePacker\TexturePacker -dupecheck -input themes\curial -output ..\..\project\Win32BuildSetup\BUILD_WIN32\application\addons\skin.estuary\media\curial.xbt
START /B /WAIT ..\..\Tools\TexturePacker\TexturePacker -dupecheck -input themes\green -output ..\..\project\Win32BuildSetup\BUILD_WIN32\application\addons\skin.estuary\media\green.xbt
START /B /WAIT ..\..\Tools\TexturePacker\TexturePacker -dupecheck -input themes\orange -output ..\..\project\Win32BuildSetup\BUILD_WIN32\application\addons\skin.estuary\media\orange.xbt
START /B /WAIT ..\..\Tools\TexturePacker\TexturePacker -dupecheck -input themes\pink -output ..\..\project\Win32BuildSetup\BUILD_WIN32\application\addons\skin.estuary\media\pink.xbt
START /B /WAIT ..\..\Tools\TexturePacker\TexturePacker -dupecheck -input themes\maroon -output ..\..\project\Win32BuildSetup\BUILD_WIN32\application\addons\skin.estuary\media\maroon.xbt
START /B /WAIT ..\..\Tools\TexturePacker\TexturePacker -dupecheck -input themes\charcoal -output ..\..\project\Win32BuildSetup\BUILD_WIN32\application\addons\skin.estuary\media\charcoal.xbt

ECHO ----------------------------------------
ECHO XBT Texture Files Created...
ECHO Building Skin Directory...
xcopy "..\skin.estuary" "..\..\project\Win32BuildSetup\BUILD_WIN32\application\addons\skin.estuary" /E /Q /I /Y /EXCLUDE:exclude.txt

del exclude.txt
