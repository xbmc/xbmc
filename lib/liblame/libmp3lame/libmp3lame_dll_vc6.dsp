# Microsoft Developer Studio Project File - Name="libmp3lame DLL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libmp3lame DLL - Win32 Release
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "libmp3lame_dll_vc6.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "libmp3lame_dll_vc6.mak" CFG="libmp3lame DLL - Win32 Release"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "libmp3lame DLL - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libmp3lame DLL - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libmp3lame DLL - Win32 Release NASM" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libmp3la"
# PROP BASE Intermediate_Dir "libmp3la"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\output\Release"
# PROP Intermediate_Dir "..\obj\Release\libmp3lameDLL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /Ob2 /D "WIN32" /D "HAVE_CONFIG_H" /D "NDEBUG" /D "_WINDOWS" /YX /Gs1024 /FD /GAy /QIfdiv /QI0f /c
# ADD CPP /nologo /MD /W3 /O2 /Ob2 /I "../libmp3lame" /I "../" /I "../mpglib" /I "../include" /I ".." /D "NDEBUG" /D "_WINDOWS" /D "HAVE_MPGLIB" /D "WIN32" /D "HAVE_CONFIG_H" /Gs1024 /FD /GAy /QIfdiv /QI0f /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:none /machine:I386 /def:"..\include\lame.def" /force /out:"..\output\Release\libmp3lame.dll" /opt:NOWIN98
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\output\Debug"
# PROP Intermediate_Dir "..\obj\Debug\libmp3lameDLL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "HAVE_CONFIG_H" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Zi /Od /I "../libmp3lame" /I "../" /I "../mpglib" /I "../include" /I ".." /D "_DEBUG" /D "_WINDOWS" /D "HAVE_MPGLIB" /D "WIN32" /D "HAVE_CONFIG_H" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /def:"..\include\lame.def" /force /out:"..\output\Debug\libmp3lame.dll" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none /map /nodefaultlib

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libmp3la"
# PROP BASE Intermediate_Dir "libmp3la"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\output\Release_NASM"
# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /Ob2 /I "../" /I "../mpglib" /I "../include" /I ".." /D "NDEBUG" /D "_WINDOWS" /D "HAVE_MPGLIB" /D "WIN32" /D "HAVE_CONFIG_H" /Gs1024 /FD /GAy /QIfdiv /QI0f /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MD /W3 /O2 /Ob2 /I "../libmp3lame" /I "../" /I "../mpglib" /I "../include" /I ".." /D "NDEBUG" /D "_WINDOWS" /D "HAVE_MPGLIB" /D "HAVE_CONFIG_H" /D "HAVE_NASM" /D "MMX_choose_table" /D "WIN32" /D "_CRT_SECURE_NO_DEPRECATE" /Gs1024 /FD /GAy /QIfdiv /QI0f /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:none /machine:I386 /def:"..\include\lame.def" /force /out:"..\output\Release_NASM\libmp3lame.dll" /opt:NOWIN98
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "libmp3lame DLL - Win32 Release"
# Name "libmp3lame DLL - Win32 Debug"
# Name "libmp3lame DLL - Win32 Release NASM"
# Begin Group "encoder sources"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=.\bitstream.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\encoder.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fft.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gain_analysis.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\id3tag.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lame.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mpglib_interface.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\newmdct.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\presets.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\psymodel.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\quantize.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\quantize_pvt.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\reservoir.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\set_get.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tables.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\takehiro.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\util.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vbrquantize.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\VbrTag.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\version.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# End Group
# Begin Group "encoder header"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\bitstream.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\encoder.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fft.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gain_analysis.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\id3tag.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\l3side.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\lame-analysis.h"

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lame_global_flags.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lameerror.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\machine.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\newmdct.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\psymodel.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\quantize.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\quantize_pvt.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\reservoir.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\set_get.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tables.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\util.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vbrquantize.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\VbrTag.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\version.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\i386\choose_table.nas

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"
# Begin Custom Build - Assembling $(InputName)...
InputDir=.\i386
IntDir=.\..\obj\Release_NASM\libmp3lameDLL
InputPath=.\i386\choose_table.nas
InputName=choose_table

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f win32 -i $(InputDir)/ -DWIN32 $(InputPath) -o   $(IntDir)/$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\cpu_feat.nas

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"
# Begin Custom Build - Assembling $(InputName)...
InputDir=.\i386
IntDir=.\..\obj\Release_NASM\libmp3lameDLL
InputPath=.\i386\cpu_feat.nas
InputName=cpu_feat

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f win32 -i $(InputDir)/ -DWIN32 $(InputPath) -o   $(IntDir)/$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\fft.nas

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\fft3dn.nas

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"
# Begin Custom Build - Assembling $(InputName)...
InputDir=.\i386
IntDir=.\..\obj\Release_NASM\libmp3lameDLL
InputPath=.\i386\fft3dn.nas
InputName=fft3dn

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f win32 -i $(InputDir)/ -DWIN32 $(InputPath) -o   $(IntDir)/$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\fftfpu.nas

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\fftsse.nas

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"
# Begin Custom Build - Assembling $(InputName)...
InputDir=.\i386
IntDir=.\..\obj\Release_NASM\libmp3lameDLL
InputPath=.\i386\fftsse.nas
InputName=fftsse

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f win32 -i $(InputDir)/ -DWIN32 $(InputPath) -o   $(IntDir)/$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\ffttbl.nas

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\scalar.nas

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# End Group
# Begin Group "decoder sources"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\mpglib\common.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\dct64_i386.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\decode_i386.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\interface.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\layer1.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\layer2.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\layer3.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\tabinit.c

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lameDLL"

!ENDIF 

# End Source File
# End Group
# Begin Group "decoder header"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=..\mpglib\common.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\dct64_i386.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\decode_i386.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\huffman.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\interface.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\layer1.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\layer2.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\layer3.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\mpg123.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\mpglib.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\mpglib\tabinit.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=..\configMS.h

!IF  "$(CFG)" == "libmp3lame DLL - Win32 Release"

# Begin Custom Build - Performing Custom Build Step on $(InputName)
InputPath=..\configMS.h

"..\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy ..\configMS.h ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Debug"

# Begin Custom Build - Performing Custom Build Step on $(InputName)
InputPath=..\configMS.h

"..\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy ..\configMS.h ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libmp3lame DLL - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\include\lame.def
# PROP Exclude_From_Build 1
# End Source File
# End Target
# End Project
