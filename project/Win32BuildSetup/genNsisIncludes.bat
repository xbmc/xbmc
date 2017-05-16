@ECHO OFF
rem Application for Windows install script
rem Copyright (C) 2005-2015 Team XBMC
rem http://kodi.tv

rem Script by chadoe
rem This script generates NullSoft NSIS installer include files for application's add-ons
rem "SectionIn" defines on what level the section is selected by default
rem 1. Full / 2. Normal  / 3. Minimal

IF EXIST *-addons.nsi del *-addons.nsi > NUL
SETLOCAL ENABLEDELAYEDEXPANSION

SET Counter=1
IF EXIST BUILD_WIN32\addons\pvr.* (
  ECHO SectionGroup "PVR Add-ons" SecPvrAddons >> pvr-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\pvr.*') DO (
    FOR /f "delims=<" %%N in ('powershell.exe -noprofile -ExecutionPolicy Unrestricted -command "& {[xml]$a = get-content BUILD_WIN32\addons\%%P\addon.xml;$a.addon.name}"') do (
      ECHO Section "%%N" SecPvrAddons!Counter! >> pvr-addons.nsi
      ECHO SectionIn 1 2 >> pvr-addons.nsi
      ECHO SetOutPath "$INSTDIR\addons\%%P" >> pvr-addons.nsi
      ECHO File /r "${app_root}\addons\%%P\*.*" >> pvr-addons.nsi
      ECHO SectionEnd >> pvr-addons.nsi
      SET /A Counter = !Counter! + 1
      )
    )
  ECHO SectionGroupEnd >> pvr-addons.nsi
)

SET Counter=1
IF EXIST BUILD_WIN32\addons\game.libretro.* (
  ECHO SectionGroup "Game Add-ons" SecGameAddons >> game-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\game.libretro.*') DO (
    FOR /f "delims=<" %%N in ('powershell.exe -ExecutionPolicy Unrestricted -command "& {[xml]$a = get-content BUILD_WIN32\addons\%%P\addon.xml;$a.addon.name}"') do (
      ECHO Section "%%N" SecGameAddons!Counter! >> game-addons.nsi
      ECHO SectionIn 1 2 >> game-addons.nsi
      ECHO SetOutPath "$INSTDIR\addons\%%P" >> game-addons.nsi
      ECHO File /r "${app_root}\addons\%%P\*.*" >> game-addons.nsi
      ECHO SectionEnd >> game-addons.nsi
      SET /A Counter = !Counter! + 1
      )
    )
  ECHO SectionGroupEnd >> game-addons.nsi
)

SET Counter=1
IF EXIST BUILD_WIN32\addons\audiodecoder.* (
  ECHO SectionGroup "Audio Decoder Add-ons" SecAudioDecoderAddons >> audiodecoder-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\audiodecoder.*') DO (
    FOR /f "delims=<" %%N in ('powershell.exe -noprofile -ExecutionPolicy Unrestricted -command "& {[xml]$a = get-content BUILD_WIN32\addons\%%P\addon.xml;$a.addon.name}"') do (
      ECHO Section "%%N" SecAudioDecoderAddons!Counter! >> audiodecoder-addons.nsi
      ECHO SectionIn 1 >> audiodecoder-addons.nsi
      ECHO SetOutPath "$INSTDIR\addons\%%P" >> audiodecoder-addons.nsi
      ECHO File /r "${app_root}\addons\%%P\*.*" >> audiodecoder-addons.nsi
      ECHO SectionEnd >> audiodecoder-addons.nsi
      SET /A Counter = !Counter! + 1
      )
    )
  ECHO SectionGroupEnd >> audiodecoder-addons.nsi
)

SET Counter=1
IF EXIST BUILD_WIN32\addons\audioencoder.* (
  ECHO SectionGroup "Audio Encoder Add-ons" SecAudioEncoderAddons >> audioencoder-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\audioencoder.*') DO (
    FOR /f "delims=<" %%N in ('powershell.exe -noprofile -ExecutionPolicy Unrestricted -command "& {[xml]$a = get-content BUILD_WIN32\addons\%%P\addon.xml;$a.addon.name}"') do (
      ECHO Section "%%N" SecAudioEncoderAddons!Counter! >> audioencoder-addons.nsi
      ECHO SectionIn 1 2 3 >> audioencoder-addons.nsi
      ECHO SetOutPath "$INSTDIR\addons\%%P" >> audioencoder-addons.nsi
      ECHO File /r "${app_root}\addons\%%P\*.*" >> audioencoder-addons.nsi
      ECHO SectionEnd >> audioencoder-addons.nsi
      SET /A Counter = !Counter! + 1
      )
    )
  ECHO SectionGroupEnd >> audioencoder-addons.nsi
)

SET Counter=1
IF EXIST BUILD_WIN32\addons\adsp.* (
  ECHO SectionGroup "Audio DSP Add-ons" SecAudioDSPAddons >> audiodsp-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\adsp.*') DO (
    FOR /f "delims=<" %%N in ('powershell.exe -noprofile -ExecutionPolicy Unrestricted -command "& {[xml]$a = get-content BUILD_WIN32\addons\%%P\addon.xml;$a.addon.name}"') do (
      ECHO Section "%%N" SecAudioDSPAddons!Counter! >> audiodsp-addons.nsi
      ECHO SectionIn 1 2 >> audiodsp-addons.nsi
      ECHO SetOutPath "$INSTDIR\addons\%%P" >> audiodsp-addons.nsi
      ECHO File /r "${app_root}\addons\%%P\*.*" >> audiodsp-addons.nsi
      ECHO SectionEnd >> audiodsp-addons.nsi
      SET /A Counter = !Counter! + 1
      )
    )
  ECHO SectionGroupEnd >> audiodsp-addons.nsi
)

SET Counter=1
IF EXIST BUILD_WIN32\addons\screensaver.* (
  ECHO SectionGroup "Screensaver Add-ons" SecScreensaverAddons >> screensaver-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\screensaver.*') DO (
    FOR /f "delims=<" %%N in ('powershell.exe -noprofile -ExecutionPolicy Unrestricted -command "& {[xml]$a = get-content BUILD_WIN32\addons\%%P\addon.xml;$a.addon.name}"') do (
      ECHO Section "%%N" SecScreensaverAddons!Counter! >> screensaver-addons.nsi
      ECHO SectionIn 1 2 3 >> screensaver-addons.nsi
      ECHO SetOutPath "$INSTDIR\addons\%%P" >> screensaver-addons.nsi
      ECHO File /r "${app_root}\addons\%%P\*.*" >> screensaver-addons.nsi
      ECHO SectionEnd >> screensaver-addons.nsi
      SET /A Counter = !Counter! + 1
      )
    )
  ECHO SectionGroupEnd >> screensaver-addons.nsi
)

SET Counter=1
IF EXIST BUILD_WIN32\addons\imagedecoder.* (
  ECHO SectionGroup "Image decoder Add-ons" SecImageDecoderAddons >> imagedecoder-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\imagedecoder.*') DO (
    FOR /f "delims=<" %%N in ('powershell.exe -noprofile -ExecutionPolicy Unrestricted -command "& {[xml]$a = get-content BUILD_WIN32\addons\%%P\addon.xml;$a.addon.name}"') do (
      ECHO Section "%%N" SecImageDecoderAddons!Counter! >> imagedecoder-addons.nsi
      ECHO SectionIn 1 2 3 >> imagedecoder-addons.nsi
      ECHO SetOutPath "$INSTDIR\addons\%%P" >> imagedecoder-addons.nsi
      ECHO File /r "${app_root}\addons\%%P\*.*" >> imagedecoder-addons.nsi
      ECHO SectionEnd >> imagedecoder-addons.nsi
      SET /A Counter = !Counter! + 1
      )
    )
  ECHO SectionGroupEnd >> imagedecoder-addons.nsi
)

SET Counter=1
IF EXIST BUILD_WIN32\addons\vfs.* (
  ECHO SectionGroup "VFS Add-ons" SecVFSAddons >> vfs-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\vfs.*') DO (
    FOR /f "delims=<" %%N in ('powershell.exe -noprofile -ExecutionPolicy Unrestricted -command "& {[xml]$a = get-content BUILD_WIN32\addons\%%P\addon.xml;$a.addon.name}"') do (
      ECHO Section "%%N" SecVFSAddons!Counter! >> vfs-addons.nsi
      ECHO SectionIn 1 2 3 >> vfs-addons.nsi
      ECHO SetOutPath "$INSTDIR\addons\%%P" >> vfs-addons.nsi
      ECHO File /r "${app_root}\addons\%%P\*.*" >> vfs-addons.nsi
      ECHO SectionEnd >> vfs-addons.nsi
      SET /A Counter = !Counter! + 1
      )
    )
  ECHO SectionGroupEnd >> vfs-addons.nsi
)

SET Counter=1
IF EXIST BUILD_WIN32\addons\visualization.* (
  ECHO SectionGroup "Visualization Add-ons" SecVisualizationAddons >> visualization-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\visualization.*') DO (
    FOR /f "delims=<" %%N in ('powershell.exe -noprofile -ExecutionPolicy Unrestricted -command "& {[xml]$a = get-content BUILD_WIN32\addons\%%P\addon.xml;$a.addon.name}"') do (
      ECHO Section "%%N" SecVisualizationAddons!Counter! >> visualization-addons.nsi
      ECHO SectionIn 1 2 3 >> visualization-addons.nsi
      ECHO SetOutPath "$INSTDIR\addons\%%P" >> visualization-addons.nsi
      ECHO File /r "${app_root}\addons\%%P\*.*" >> visualization-addons.nsi
      ECHO SectionEnd >> visualization-addons.nsi
      SET /A Counter = !Counter! + 1
      )
    )
  ECHO SectionGroupEnd >> visualization-addons.nsi
)

SET Counter=1
IF EXIST BUILD_WIN32\addons\inputstream.* (
  ECHO SectionGroup "Inputstream Add-ons" SecInputstreamAddons >> inputstream-addons.nsi
  FOR /F "tokens=*" %%P IN ('dir /B /AD BUILD_WIN32\addons\inputstream.*') DO (
    FOR /f "delims=<" %%N in ('powershell.exe -noprofile -ExecutionPolicy Unrestricted -command "& {[xml]$a = get-content BUILD_WIN32\addons\%%P\addon.xml;$a.addon.name}"') do (
      ECHO Section "%%N" SecInputstreamAddons!Counter! >> inputstream-addons.nsi
      ECHO SectionIn 1 2 >> inputstream-addons.nsi
      ECHO SetOutPath "$INSTDIR\addons\%%P" >> inputstream-addons.nsi
      ECHO File /r "${app_root}\addons\%%P\*.*" >> inputstream-addons.nsi
      ECHO SectionEnd >> inputstream-addons.nsi
      SET /A Counter = !Counter! + 1
      )
    )
  ECHO SectionGroupEnd >> inputstream-addons.nsi
)

ENDLOCAL