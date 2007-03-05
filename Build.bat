@echo off
ECHO ------------------------------
echo Creating pm3_concept Build Folder
rmdir BUILD /S /Q
md BUILD

ECHO ------------------------------
ECHO Creating XPR Files...
media\XBMCTex.exe -input media -quality max -output media -noprotect

ECHO ------------------------------
ECHO Copying XPR Files...
xcopy media\Textures.xpr "BUILD\pm3_concept\media\" /Q /I /Y

ECHO ------------------------------
ECHO Cleaning Up...
del media\*.xpr

ECHO ------------------------------
ECHO Building Skin Directory...
xcopy "1080i" "BUILD\pm3_concept\1080i" /E /Q /I /Y
xcopy "720p" "BUILD\pm3_concept\720p" /E /Q /I /Y
xcopy "NTSC" "BUILD\pm3_concept\NTSC" /E /Q /I /Y
xcopy "NTSC16x9" "BUILD\pm3_concept\NTSC16x9" /E /Q /I /Y
xcopy "PAL" "BUILD\pm3_concept\PAL" /E /Q /I /Y
xcopy "PAL16x9" "BUILD\pm3_concept\PAL16x9" /E /Q /I /Y
xcopy "fonts" "BUILD\pm3_concept\fonts" /E /Q /I /Y
xcopy "*.xml" "BUILD\pm3_concept\" /Q /I /Y

copy *.xml "BUILD\pm3_concept\"
copy *.txt "BUILD\pm3_concept\"

ECHO ------------------------------
ECHO Removing SVN directories from build
FOR /R BUILD %%d IN (SVN) DO @RD /S /Q "%%d" 2>NUL

echo Build Complete - Scroll Up to check for errors.
echo Final build is located in the BUILD directory
echo ftp the pm3_concept folder in the build dir to your xbox
pause
