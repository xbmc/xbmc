@ECHO OFF
rem XBMC for Windows install script
rem Copyright (C) 2005-2008 Team XBMC
rem http://xbmc.org

rem Script by chadoe
rem This script generates nullsoft installer include files for xbmc's languages, skins, scripts and plugins

rem languages
IF EXIST languages.nsi del languages.nsi > NUL
SETLOCAL ENABLEDELAYEDEXPANSION
SET Counter=1
FOR /F "tokens=*" %%S IN ('dir /B /AD BUILD_WIN32\Xbmc\language') DO (
  rem English is already included as default language
  IF "%%S" NEQ "English" (
    ECHO Section "%%S" SecLanguage!Counter! >> languages.nsi
    ECHO SectionIn 1 #section is in installtype Full >> languages.nsi
    ECHO SetOutPath "$INSTDIR\language\%%S" >> languages.nsi
    ECHO File /r "${xbmc_root}\Xbmc\language\%%S\*.*" >> languages.nsi
    ECHO SectionEnd >> languages.nsi
    SET /A Counter = !Counter! + 1
  )
)
ENDLOCAL