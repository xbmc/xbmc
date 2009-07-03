@ECHO OFF
CLS
COLOR 1B

:PluginType
SET PluginType=programs

:Begin
:: Set plugin name based on current directory
FOR /F "Delims=" %%D IN ('ECHO %CD%') DO SET PluginName=%%~nD

:: Set window title
TITLE %PluginName% Build Script!

:MakeBuildFolder
:: Create Build folder
ECHO ----------------------------------------------------------------------
ECHO.
ECHO Creating \BUILD\%PluginName%\ folder . . .
IF EXIST BUILD (
    RD BUILD /S /Q
)
MD BUILD
ECHO.

:GetRevision
:: Extract Revision # and SET %Revision% variable
ECHO ----------------------------------------------------------------------
ECHO.
ECHO Extracting revision number . . .
ECHO.
FOR /F "Tokens=2* Delims=]" %%R IN ('FIND /v /n "&_&_&_&" ".svn\entries" ^| FIND "[11]"') DO SET Revision=%%R

:MakeExcludeFile
:: Create exclude file
ECHO ----------------------------------------------------------------------
ECHO.
ECHO Creating exclude.txt file . . .
ECHO.
ECHO .svn>"BUILD\exclude.txt"
ECHO Thumbs.db>>"BUILD\exclude.txt"
ECHO Desktop.ini>>"BUILD\exclude.txt"

:MakeReleaseBuild
:: Create release build
ECHO ----------------------------------------------------------------------
ECHO.
ECHO Copying required files to \Build\%PluginType%\%PluginName%\ folder . . .
XCOPY resources "BUILD\%PluginType%\%PluginName%\resources" /E /Q /I /Y /EXCLUDE:BUILD\exclude.txt
XCOPY installerAPI "BUILD\%PluginType%\%PluginName%\installerAPI" /E /Q /I /Y /EXCLUDE:BUILD\exclude.txt
COPY default.py "BUILD\%PluginType%\%PluginName%\"
COPY default.tbn "BUILD\%PluginType%\%PluginName%\"
ECHO.

:Cleanup
:: Delete exclude.txt file
ECHO ----------------------------------------------------------------------
ECHO.
ECHO Cleaning up . . .
DEL "BUILD\exclude.txt"
ECHO.
ECHO.

:Finish
:: Notify user of completion
ECHO ======================================================================
ECHO.
ECHO Build Complete - Scroll up to check for errors.
ECHO.
ECHO Final build is located in the \BUILD\%PluginType%\ folder.
ECHO.
ECHO copy: \%PluginName%\ folder from the \BUILD\%PluginType%\ folder.
ECHO to: /XBMC/plugins/%PluginType%/ folder.
ECHO.
ECHO ======================================================================
ECHO.
PAUSE
