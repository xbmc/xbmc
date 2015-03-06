@ECHO OFF
rem Application for Windows install script
rem Copyright (C) 2005-2013 Team XBMC
rem http://xbmc.org

rem Script by chadoe
rem This script generates NullSoft NSIS installer include files for application's add-ons
rem and addons
rem 1. Full / 2. Normal  / 3. Minimal
rem languages

IF EXIST *-addons.nsi del *-addons.nsi > NUL
SETLOCAL ENABLEDELAYEDEXPANSION

SET Counter=1
IF EXIST BUILD_WIN32\addons\pvr.* (
  ECHO SectionGroup "PVR Add-ons" SecPvrAddons >> pvr-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\pvr.*') DO (
    SET "output=%%P"
    SET output=!output:pvr.=!
    ECHO Section "!output!" SecPvrAddons!Counter! >> pvr-addons.nsi
    ECHO SectionIn 1 #section is in installtype Full >> pvr-addons.nsi
    ECHO SetOutPath "$INSTDIR\addons\%%P" >> pvr-addons.nsi
    ECHO File /r "${app_root}\addons\%%P\*.*" >> pvr-addons.nsi
    ECHO SectionEnd >> pvr-addons.nsi
    SET /A Counter = !Counter! + 1
    )
  ECHO SectionGroupEnd >> pvr-addons.nsi
)

SET Counter=1
IF EXIST BUILD_WIN32\addons\audiodecoder.* (
  ECHO SectionGroup "Audio Decoder Add-ons" SecAudioDecoderAddons >> audiodecoder-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\audiodecoder.*') DO (
    SET "output=%%P"
    SET output=!output:audiodecoder.=!
    ECHO Section "!output!" SecAudioDecoderAddons!Counter! >> audiodecoder-addons.nsi
    ECHO SectionIn 1 2 3 #section is in installtype Full >> audiodecoder-addons.nsi
    ECHO SetOutPath "$INSTDIR\addons\%%P" >> audiodecoder-addons.nsi
    ECHO File /r "${app_root}\addons\%%P\*.*" >> audiodecoder-addons.nsi
    ECHO SectionEnd >> audiodecoder-addons.nsi
    SET /A Counter = !Counter! + 1
    )
  ECHO SectionGroupEnd >> audiodecoder-addons.nsi
)

SET Counter=1
IF EXIST BUILD_WIN32\addons\audioencoder.* (
  ECHO SectionGroup "Audio Encoder Add-ons" SecAudioEncoderAddons >> audioencoder-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\audioencoder.*') DO (
    SET "output=%%P"
    SET output=!output:audioencoder.=!
    ECHO Section "!output!" SecAudioEncoderAddons!Counter! >> audioencoder-addons.nsi
    ECHO SectionIn 1 2 3 #section is in installtype Full >> audioencoder-addons.nsi
    ECHO SetOutPath "$INSTDIR\addons\%%P" >> audioencoder-addons.nsi
    ECHO File /r "${app_root}\addons\%%P\*.*" >> audioencoder-addons.nsi
    ECHO SectionEnd >> audioencoder-addons.nsi
    SET /A Counter = !Counter! + 1
    )
  ECHO SectionGroupEnd >> audioencoder-addons.nsi
)

SET Counter=1
IF EXIST BUILD_WIN32\addons\skin.* (
  ECHO SectionGroup "Skin Add-ons" SecSkinAddons >> skin-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\skin.*') DO (
    SET "output=%%P"
    SET output=!output:skin.=!
    ECHO Section "!output!" SecSkinAddons!Counter! >> skin-addons.nsi
    ECHO SectionIn 1 #section is in installtype Full >> skin-addons.nsi
    ECHO SetOutPath "$INSTDIR\addons\%%P" >> skin-addons.nsi
    ECHO File /r "${app_root}\addons\%%P\*.*" >> skin-addons.nsi
    ECHO SectionEnd >> skin-addons.nsi
    SET /A Counter = !Counter! + 1
    )
  ECHO SectionGroupEnd >> skin-addons.nsi
)

ENDLOCAL