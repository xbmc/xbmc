@echo off
ECHO ----------------------------------------
echo Creating Project Mayhem III Build Folder
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
xcopy "media\Textures.xbt" "BUILD\Project Mayhem III\media\" /Q /I /Y

ECHO ----------------------------------------
ECHO Cleaning Up...
del "media\Textures.xbt"

ECHO ----------------------------------------
ECHO XBT Texture Files Created...
ECHO Building Skin Directory...
xcopy "720p" "BUILD\Project Mayhem III\720p" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "1080i" "BUILD\Project Mayhem III\1080i" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "fonts" "BUILD\Project Mayhem III\fonts" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "NTSC" "BUILD\Project Mayhem III\NTSC" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "NTSC16x9" "BUILD\Project Mayhem III\NTSC16x9" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "PAL" "BUILD\Project Mayhem III\PAL" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "PAL16x9" "BUILD\Project Mayhem III\PAL16x9" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "sounds\*.*" "BUILD\Project Mayhem III\sounds\" /Q /I /Y /EXCLUDE:exclude.txt
xcopy "colors\*.*" "BUILD\Project Mayhem III\colors\" /Q /I /Y /EXCLUDE:exclude.txt

del exclude.txt

copy *.xml "BUILD\Project Mayhem III\"
copy *.txt "BUILD\Project Mayhem III\"