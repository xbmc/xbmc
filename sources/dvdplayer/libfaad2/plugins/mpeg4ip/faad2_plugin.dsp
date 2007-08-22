# Microsoft Developer Studio Project File - Name="faad2_plugin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=faad2_plugin - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "faad2_plugin.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "faad2_plugin.mak" CFG="faad2_plugin - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "faad2_plugin - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "faad2_plugin - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "faad2_plugin - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../include" /I "/mpeg4ip/include" /I "/mpeg4ip/player/src" /I "/mpeg4ip/player/lib/mp4util" /I "/mpeg4ip/lib/sdp" /I "/mpeg4ip/lib/SDL/include" /I "/mpeg4ip/lib/mp4v2" /D "_WINDOWS" /D "_WIN32" /D "WIN32" /D "NDEBUG" /YX /FD /I /mpeg4ip/include" /I /mpeg4ip/player/src" /I /mpeg4ip/lib/sdp" /I /mpeg4ip/player/lib/mp4util" /I /mpeg4ip/lib/SDL/include" " " " " " /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib \mpeg4ip\player\lib\mp4util\Release\libmp4util60.lib \mpeg4ip\lib\bitstream\Release\bitstream60.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libcmt.lib" /nodefaultlib:"libc.lib"

!ELSEIF  "$(CFG)" == "faad2_plugin - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../../include" /I "/mpeg4ip/include" /I "/mpeg4ip/player/src" /I "/mpeg4ip/player/lib/mp4util" /I "/mpeg4ip/lib/sdp" /I "/mpeg4ip/lib/SDL/include" /I "/mpeg4ip/lib/mp4v2" /D "_WINDOWS" /D "_WIN32" /D "WIN32" /D "_DEBUG" /YX /FD /I /mpeg4ip/lib/mp4v2" /I /mpeg4ip/lib/SDL" /I /mpeg4ip/include" /I /mpeg4ip/player/src" /I /mpeg4ip/lib/sdp" /I /mpeg4ip/player/lib/mp4util" /I /mpeg4ip/lib/SDL/include" " " " " " " " /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib \mpeg4ip\player\lib\mp4util\Debug\libmp4util60.lib \mpeg4ip\lib\bitstream\Debug\bitstream60.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libcd.lib" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "faad2_plugin - Win32 Release"
# Name "faad2_plugin - Win32 Debug"
# Begin Source File

SOURCE=.\aa_file.cpp
# End Source File
# Begin Source File

SOURCE=.\aa_file.h
# End Source File
# Begin Source File

SOURCE=.\faad2.cpp
# End Source File
# Begin Source File

SOURCE=.\faad2.h
# End Source File
# End Target
# End Project
