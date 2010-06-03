@echo off
ECHO ----------------------------------------
echo Creating Confluence Build Folder
rmdir BUILD /S /Q
md BUILD

Echo .svn>exclude.txt
Echo Thumbs.db>>exclude.txt
Echo Desktop.ini>>exclude.txt
Echo dsstdfx.bin>>exclude.txt
Echo exclude.txt>>exclude.txt

ECHO ----------------------------------------
ECHO Creating XBT File...
START /B /WAIT ..\..\Tools\TexturePacker\TexturePacker -input media -output media\Textures.xbt

ECHO ----------------------------------------
ECHO Copying XBT File...
xcopy "media\Textures.xbt" "BUILD\Confluence\media\" /Q /I /Y

ECHO ----------------------------------------
ECHO Cleaning Up...
del "media\Textures.xbt"

ECHO ----------------------------------------
ECHO XBT Texture Files Created...
ECHO Building Skin Directory...
xcopy "720p" "BUILD\Confluence\720p" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "fonts" "BUILD\Confluence\fonts" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "backgrounds" "BUILD\Confluence\backgrounds" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "scripts" "BUILD\Confluence\scripts" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "sounds\*.*" "BUILD\Confluence\sounds\" /Q /I /Y /EXCLUDE:exclude.txt
xcopy "colors\*.*" "BUILD\Confluence\colors\" /Q /I /Y /EXCLUDE:exclude.txt
xcopy "language" "BUILD\Confluence\language" /E /Q /I /Y /EXCLUDE:exclude.txt

del exclude.txt

copy *.xml "BUILD\Confluence\"
copy *.txt "BUILD\Confluence\"
copy icon.png "BUILD\Confluence\"