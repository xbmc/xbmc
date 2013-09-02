@ECHO OFF
rem XBMC for Windows install script
rem Copyright (C) 2005-2013 Team XBMC
rem http://xbmc.org

rem Script by chadoe
rem This script generates nullsoft installer include files for xbmc's languages
rem and pvr addons
rem 1. Full / 2. Normal  / 3. Minimal
rem languages

IF EXIST xbmc-pvr-addons.nsi del xbmc-pvr-addons.nsi > NUL
IF EXIST skins.nsi del skins.nsi > NUL
SETLOCAL ENABLEDELAYEDEXPANSION

SET Counter=1
FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\Xbmc\xbmc-pvr-addons') DO (
  SET "output=%%P"
  SET output=!output:pvr.=!
  ECHO Section !output! SecPvrAddons!Counter! >> xbmc-pvr-addons.nsi
  ECHO SectionIn 1 #section is in installtype Full >> xbmc-pvr-addons.nsi
  ECHO SetOutPath "$INSTDIR\addons\%%P" >> xbmc-pvr-addons.nsi
  ECHO File /r "${xbmc_root}\Xbmc\xbmc-pvr-addons\%%P\*.*" >> xbmc-pvr-addons.nsi
  ECHO SectionEnd >> xbmc-pvr-addons.nsi
  SET /A Counter = !Counter! + 1
)

SET Counter=1
FOR /F "tokens=*" %%R IN ('dir /B /AD BUILD_WIN32\Xbmc\addons\skin*') DO (
  SET "output=%%R"
  SET output=!output:skin.=!
  rem Confluence is already included as default skin
  IF "%%R" NEQ "skin.confluence" (
    ECHO Section !output! SecSkins!Counter! >> skins.nsi
    ECHO SectionIn 1 #section is in installtype Full >> skins.nsi
    ECHO SetOutPath "$INSTDIR\addons\%%R" >> skins.nsi
    ECHO File /r "${xbmc_root}\Xbmc\addons\%%R\*.*" >> skins.nsi
    ECHO SectionEnd >> skins.nsi
    SET /A Counter = !Counter! + 1
  )
)
ENDLOCAL