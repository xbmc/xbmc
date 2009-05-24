@echo off
ECHO ----------------------------------------
echo Creating PM3.HD Build Folder
rmdir BUILD /S /Q
md BUILD

Echo .svn>exclude.txt
Echo Thumbs.db>>exclude.txt
Echo Desktop.ini>>exclude.txt
Echo dsstdfx.bin>>exclude.txt
Echo exclude.txt>>exclude.txt

ECHO ----------------------------------------
ECHO Creating XPR File...
START /B /WAIT ..\..\Tools\XBMCTex\XBMCTex -input media -output media -noprotect

ECHO ----------------------------------------
ECHO Copying XPR File...
xcopy "media\Textures.xpr" "BUILD\PM3.HD\media\" /Q /I /Y

ECHO ----------------------------------------
ECHO Cleaning Up...
del "media\Textures.xpr"

ECHO ----------------------------------------
ECHO XPR Texture Files Created...
ECHO Building Skin Directory...
xcopy "720p" "BUILD\PM3.HD\720p" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "fonts" "BUILD\PM3.HD\fonts" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "backgrounds" "BUILD\PM3.HD\backgrounds" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "sounds\*.*" "BUILD\PM3.HD\sounds\" /Q /I /Y /EXCLUDE:exclude.txt
xcopy "colors\*.*" "BUILD\PM3.HD\colors\" /Q /I /Y /EXCLUDE:exclude.txt
xcopy "language" "BUILD\PM3.HD\language" /E /Q /I /Y /EXCLUDE:exclude.txt

del exclude.txt

copy *.xml "BUILD\PM3.HD\"
copy *.txt "BUILD\PM3.HD\"