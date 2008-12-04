@ECHO OFF
CLS
COLOR 1B

:Begin
:: Set skin name based on skin.xml setting, if doesn't work use current directory
FOR /F "Tokens=3 Delims=><" %%N IN ('FIND "<skinname>" "skin.xml"') DO SET SkinName=%%N
IF "%SkinName%"=="" (
    FOR /F "Delims=" %%D IN ('ECHO %CD%') DO SET SkinName=%%~nD
)
:: Set window title
TITLE %SkinName% Build Script!

:GetRevision
:: Extract Revision # and SET %Revision% variable
CLS
ECHO ----------------------------------------------------------------------
ECHO.
ECHO Extracting revision number . . .
ECHO.
FOR /F "Tokens=2* Delims=]" %%R IN ('FIND /v /n "&_&_&_&" ".svn\entries" ^| FIND "[11]"') DO SET Revision=%%R

:GetResolutions
:: Extract Resolution and Version from skin.xml and SET the variables
ECHO ----------------------------------------------------------------------
ECHO.
ECHO Getting default resolutions and version . . .
ECHO.
FOR /F "Tokens=3 Delims=><" %%R IN ('FIND "<defaultresolution>" "skin.xml"') DO SET DefaultResolution=%%R
FOR /F "Tokens=3 Delims=><" %%R IN ('FIND "<defaultresolutionwide>" "skin.xml"') DO SET DefaultResolutionWide=%%R
FOR /F "Tokens=3 Delims=><" %%V IN ('FIND "<skinversion>" "skin.xml"') DO SET Version=%%V

:MakeBuildFolder
:: Create Build folder
ECHO ----------------------------------------------------------------------
ECHO.
ECHO Creating \BUILD\%SkinName%\ folder . . .
IF EXIST BUILD\%SkinName% (
    RD BUILD\%SkinName% /S /Q
)
MD BUILD\%SkinName%
ECHO.

:MakeExcludeFile
:: Create exclude file
ECHO ----------------------------------------------------------------------
ECHO.
ECHO Creating exclude.txt file . . .
ECHO.
ECHO .svn>"BUILD\exclude.txt"
ECHO Thumbs.db>>"BUILD\exclude.txt"
ECHO Desktop.ini>>"BUILD\exclude.txt"

:MakeXPRFile
ECHO ----------------------------------------------------------------------
ECHO.
ECHO Compressing images to .xpr
ECHO.
CALL XBMCTex.bat

:MakeReleaseBuild
:: Create release build
ECHO ----------------------------------------------------------------------
ECHO.
ECHO Copying required files to \BUILD\%SkinName%\ folder . . .
ECHO.
xcopy "720p" "BUILD\%SkinName%\720p" /E /Q /I /Y
xcopy "1080i" "BUILD\%SkinName%\1080i" /E /Q /I /Y
xcopy "colors" "BUILD\%SkinName%\colors" /E /Q /I /Y
xcopy "fonts" "BUILD\%SkinName%\fonts" /E /Q /I /Y
xcopy "NTSC" "BUILD\%SkinName%\NTSC" /E /Q /I /Y
xcopy "NTSC16x9" "BUILD\%SkinName%\NTSC16x9" /E /Q /I /Y
xcopy "PAL" "BUILD\%SkinName%\PAL" /E /Q /I /Y
xcopy "PAL16x9" "BUILD\%SkinName%\PAL16x9" /E /Q /I /Y
xcopy "sounds" "BUILD\%SkinName%\sounds" /E /Q /I /Y
xcopy "language" "BUILD\%SkinName%\language" /E /Q /I /Y
xcopy "extras" "BUILD\%SkinName%\extras" /E /Q /I /Y
xcopy "*.xml" "BUILD\%SkinName%\" /Q /I /Y
xcopy "*.txt" "BUILD\%SkinName%\" /Q /I /Y
MD "BUILD\%SkinName%\media"
copy "Media\*.xpr" "BUILD\%SkinName%\media\"

:MakeIncludeFile
:: Create revision include file
ECHO ----------------------------------------------------------------------
ECHO.
IF NOT "%DefaultResolution%"=="" (
    ECHO Making %DefaultResolution% revision.xml include file . . .
    ECHO ^<!-- %SkinName% skin revision: %Revision% - built with build.bat version 1.0 --^>> "BUILD\%SkinName%\%DefaultResolution%\revision.xml"
    ECHO ^<includes^>>> "BUILD\%SkinName%\%DefaultResolution%\revision.xml"
    ECHO   ^<include name="Revision"^>>> "BUILD\%SkinName%\%DefaultResolution%\revision.xml"
    ECHO     ^<label^>%SkinName% %Version% ^(%date%^), SVN - r%Revision%^</label^>>> "BUILD\%SkinName%\%DefaultResolution%\revision.xml"
    ECHO   ^</include^>>> "BUILD\%SkinName%\%DefaultResolution%\revision.xml"
    ECHO ^</includes^>>> "BUILD\%SkinName%\%DefaultResolution%\revision.xml"
    ECHO.
)
IF NOT "%DefaultResolutionWide%"=="" (
    IF NOT "%DefaultResolutionWide%"=="%DefaultResolution%" (
        ECHO Making %DefaultResolutionWide% revision.xml include file . . .
        ECHO ^<!-- %SkinName% skin revision: %Revision% - built with build.bat version 1.0 --^>> "BUILD\%SkinName%\%DefaultResolutionWide%\revision.xml"
        ECHO ^<includes^>>> "BUILD\%SkinName%\%DefaultResolutionWide%\revision.xml"
        ECHO   ^<include name="Revision"^>>> "BUILD\%SkinName%\%DefaultResolutionWide%\revision.xml"
        ECHO     ^<label^>%SkinName% %Version% ^(%date%^), SVN - r%Revision%^</label^>>> "BUILD\%SkinName%\%DefaultResolution%\revision.xml"
        ECHO   ^</include^>>> "BUILD\%SkinName%\%DefaultResolutionWide%\revision.xml"
        ECHO ^</includes^>>> "BUILD\%SkinName%\%DefaultResolutionWide%\revision.xml"
        ECHO.
    )
)

:Cleanup
:: Delete exclude.txt file
ECHO ----------------------------------------------------------------------
ECHO.
ECHO Cleaning up . . .
DEL "BUILD\exclude.txt"
DEL "Media\*.xpr"
ECHO.
ECHO.

:Finish
:: Notify user of completion
ECHO ======================================================================
ECHO.
ECHO Build Complete - Scroll up to check for errors.
ECHO.
ECHO Final build is located in the \BUILD\ folder.
ECHO.
ECHO copy: \%SkinName%\ folder from the \BUILD\ folder.
ECHO to: /XBMC/skin/ folder.
ECHO.
ECHO ======================================================================
ECHO.
PAUSE

:Quit

