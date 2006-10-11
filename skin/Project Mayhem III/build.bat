@echo off
ECHO ----------------------------------------
echo Creating Project Mayhem III Build Folder
rmdir BUILD /S /Q
md BUILD

ECHO ----------------------------------------
ECHO Creating XPR File...
START /B /WAIT ..\..\Tools\XBMCTex\XBMCTex -input media -quality high -output media

ECHO ----------------------------------------
ECHO Copying XPR File...
xcopy *.xpr BUILD\Project Mayhem III\media\ /Q /I /Y

ECHO ----------------------------------------
ECHO Cleaning Up...
del *.xpr

ECHO ----------------------------------------
ECHO XPR Texture Files Created...
ECHO Building Skin Directory...
xcopy "720p" "BUILD\Project Mayhem III\720p" /E /Q /I /Y
xcopy "1080i" "BUILD\Project Mayhem III\1080i" /E /Q /I /Y
xcopy "fonts" "BUILD\Project Mayhem III\fonts" /E /Q /I /Y
xcopy "NTSC" "BUILD\Project Mayhem III\NTSC" /E /Q /I /Y
xcopy "NTSC16x9" "BUILD\Project Mayhem III\NTSC16x9" /E /Q /I /Y
xcopy "PAL" "BUILD\Project Mayhem III\PAL" /E /Q /I /Y
xcopy "PAL16x9" "BUILD\Project Mayhem III\PAL16x9" /E /Q /I /Y
xcopy "sounds\*.*" "BUILD\Project Mayhem III\sounds\" /Q /I /Y 

copy *.xml BUILD\Project Mayhem III\
copy *.txt BUILD\Project Mayhem III\

ECHO ----------------------------------------
ECHO Removing CVS directories from build
FOR /R BUILD %%d IN (CVS) DO @RD /S /Q "%%d" 2>NUL

