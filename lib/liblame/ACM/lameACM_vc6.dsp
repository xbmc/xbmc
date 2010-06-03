# Microsoft Developer Studio Project File - Name="lameACM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=LAMEACM - WIN32 RELEASE
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "lameACM_vc6.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "lameACM_vc6.mak" CFG="LAMEACM - WIN32 RELEASE"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "lameACM - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "lameACM - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lameACM - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\output\Release"
# PROP Intermediate_Dir "..\obj\Release\lameACM"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Zp2 /MT /W3 /GX /Ob2 /I "../libmp3lame" /I "../include" /I ".." /I "../.." /I "../mpglib" /I "./" /I "./ddk" /D "NDEBUG" /D "_BLADEDLL" /D "_WINDOWS" /D "WIN32" /D "NOANALYSIS" /D "LAME_ACM" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 libmp3lame.lib shell32.lib url.lib gdi32.lib winmm.lib advapi32.lib user32.lib kernel32.lib libc.lib libcp.lib /nologo /subsystem:windows /dll /pdb:none /machine:I386 /nodefaultlib /def:".\lameACM.def" /out:"..\output\Release\lameACM.acm" /libpath:"..\output\Release" /opt:NOWIN98
# Begin Special Build Tool
TargetDir=\cvs\lame-398b4\lame\output\Release
SOURCE="$(InputPath)"
PostBuild_Desc=ACM config files
PostBuild_Cmds=copy lameacm.inf $(TargetDir)\*.*	copy lame_acm.xml $(TargetDir)\*.*
# End Special Build Tool

!ELSEIF  "$(CFG)" == "lameACM - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\output\Debug"
# PROP Intermediate_Dir "..\obj\Debug\lameACM"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Zp2 /MTd /W3 /GX /ZI /Od /I "../libmp3lame" /I "../include" /I ".." /I "../.." /I "../mpglib" /I "./" /I "./ddk" /D "_DEBUG" /D "_BLADEDLL" /D "_WINDOWS" /D "WIN32" /D "NOANALYSIS" /D "LAME_ACM" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libmp3lame.lib shell32.lib url.lib gdi32.lib winmm.lib advapi32.lib user32.lib kernel32.lib libcd.lib libcpd.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /def:".\lameACM.def" /out:"..\output\Debug\lameACM.acm" /pdbtype:sept /libpath:"..\output\Debug" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none /incremental:no
# Begin Special Build Tool
TargetDir=\cvs\lame-398b4\lame\output\Debug
SOURCE="$(InputPath)"
PostBuild_Desc=ACM config files
PostBuild_Cmds=copy lameacm.inf $(TargetDir)\*.*	copy lame_acm.xml $(TargetDir)\*.*
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "lameACM - Win32 Release"
# Name "lameACM - Win32 Debug"
# Begin Group "Source"

# PROP Default_Filter "c;cpp"
# Begin Source File

SOURCE=.\ACM.cpp
# End Source File
# Begin Source File

SOURCE=.\ACMStream.cpp
# End Source File
# Begin Source File

SOURCE=.\AEncodeProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\DecodeStream.cpp
# End Source File
# Begin Source File

SOURCE=.\lameACM.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# End Group
# Begin Group "Include"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\ACM.h
# End Source File
# Begin Source File

SOURCE=.\ACMStream.h
# End Source File
# Begin Source File

SOURCE=.\adebug.h
# End Source File
# Begin Source File

SOURCE=.\AEncodeProperties.h
# End Source File
# Begin Source File

SOURCE=.\DecodeStream.h
# End Source File
# End Group
# Begin Group "Resource"

# PROP Default_Filter "rc"
# Begin Source File

SOURCE=.\acm.rc
# End Source File
# Begin Source File

SOURCE=.\lame.ico
# End Source File
# End Group
# Begin Group "Install"

# PROP Default_Filter "inf;acm"
# Begin Source File

SOURCE=.\LameACM.inf
# End Source File
# End Group
# Begin Source File

SOURCE=.\readme.txt
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\TODO
# PROP Exclude_From_Build 1
# End Source File
# End Target
# End Project
