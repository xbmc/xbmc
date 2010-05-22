# Microsoft Developer Studio Project File - Name="libmp3lame" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libmp3lame - Win32 Release
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "libmp3lame_vc6.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "libmp3lame_vc6.mak" CFG="libmp3lame - Win32 Release"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "libmp3lame - Win32 Release" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE "libmp3lame - Win32 Debug" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE "libmp3lame - Win32 Release NASM" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libmp3lame - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libmp3la"
# PROP BASE Intermediate_Dir "libmp3la"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\output\Release"
# PROP Intermediate_Dir "..\obj\Release\libmp3lame"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /Ob2 /D "WIN32" /D "HAVE_CONFIG_H" /D "NDEBUG" /D "_WINDOWS" /YX /Gs1024 /FD /GAy /QIfdiv /QI0f /c
# ADD CPP /nologo /W3 /O2 /Ob2 /I "../" /I "../mpglib" /I "../include" /I ".." /D "NDEBUG" /D "_WINDOWS" /D "HAVE_MPGLIB" /D "WIN32" /D "HAVE_CONFIG_H" /Gs1024 /FD /GAy /QIfdiv /QI0f /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\output\Release\libmp3lame-static.lib"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\output\Debug"
# PROP Intermediate_Dir "..\obj\Debug\libmp3lame"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "HAVE_CONFIG_H" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /ZI /Od /I "../" /I "../mpglib" /I "../include" /I ".." /D "_DEBUG" /D "_WINDOWS" /D "HAVE_MPGLIB" /D "WIN32" /D "HAVE_CONFIG_H" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\output\Debug\libmp3lame-static.lib"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Release NASM"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libmp3la"
# PROP BASE Intermediate_Dir "libmp3la"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\output\Release_NASM"
# PROP Intermediate_Dir "..\obj\Release_NASM\libmp3lame"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /Ob2 /I "../" /I "../mpglib" /I "../include" /I ".." /D "NDEBUG" /D "_WINDOWS" /D "HAVE_MPGLIB" /D "WIN32" /D "HAVE_CONFIG_H" /Gs1024 /FD /GAy /QIfdiv /QI0f /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /W3 /O2 /Ob2 /I "../" /I "../mpglib" /I "../include" /I ".." /D "NDEBUG" /D "_WINDOWS" /D "HAVE_MPGLIB" /D "WIN32" /D "HAVE_CONFIG_H" /D "HAVE_NASM" /D "MMX_choose_table" /D "_CRT_SECURE_NO_DEPRECATE" /Gs1024 /FD /GAy /QIfdiv /QI0f /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\output\Release_NASM\libmp3lame-static.lib"

!ENDIF 

# Begin Target

# Name "libmp3lame - Win32 Release"
# Name "libmp3lame - Win32 Debug"
# Name "libmp3lame - Win32 Release NASM"
# Begin Group "Source"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=.\bitstream.c
# End Source File
# Begin Source File

SOURCE=.\encoder.c
# End Source File
# Begin Source File

SOURCE=.\fft.c
# End Source File
# Begin Source File

SOURCE=.\gain_analysis.c
# End Source File
# Begin Source File

SOURCE=.\id3tag.c
# End Source File
# Begin Source File

SOURCE=.\lame.c
# End Source File
# Begin Source File

SOURCE=.\mpglib_interface.c
# End Source File
# Begin Source File

SOURCE=.\newmdct.c
# End Source File
# Begin Source File

SOURCE=.\presets.c
# End Source File
# Begin Source File

SOURCE=.\psymodel.c
# End Source File
# Begin Source File

SOURCE=.\quantize.c
# End Source File
# Begin Source File

SOURCE=.\quantize_pvt.c
# End Source File
# Begin Source File

SOURCE=.\reservoir.c
# End Source File
# Begin Source File

SOURCE=.\set_get.c
# End Source File
# Begin Source File

SOURCE=.\tables.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\takehiro.c
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# Begin Source File

SOURCE=.\vbrquantize.c
# End Source File
# Begin Source File

SOURCE=.\VbrTag.c
# End Source File
# Begin Source File

SOURCE=.\version.c
# End Source File
# End Group
# Begin Group "Include"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\bitstream.h
# End Source File
# Begin Source File

SOURCE=..\configMS.h

!IF  "$(CFG)" == "libmp3lame - Win32 Release"

# Begin Custom Build - Performing Custom Build Step on $(InputName)
InputPath=..\configMS.h

"..\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy ..\configMS.h ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Debug"

# Begin Custom Build - Performing Custom Build Step on $(InputName)
InputPath=..\configMS.h

"..\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy ..\configMS.h ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Release NASM"

# Begin Custom Build - Performing Custom Build Step on $(InputName)
InputPath=..\configMS.h

"..\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy ..\configMS.h ..\config.h

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\encoder.h
# End Source File
# Begin Source File

SOURCE=.\fft.h
# End Source File
# Begin Source File

SOURCE=.\gain_analysis.h
# End Source File
# Begin Source File

SOURCE=.\id3tag.h
# End Source File
# Begin Source File

SOURCE=.\l3side.h
# End Source File
# Begin Source File

SOURCE=".\lame-analysis.h"
# End Source File
# Begin Source File

SOURCE=.\lame_global_flags.h
# End Source File
# Begin Source File

SOURCE=.\lameerror.h
# End Source File
# Begin Source File

SOURCE=.\machine.h
# End Source File
# Begin Source File

SOURCE=.\newmdct.h
# End Source File
# Begin Source File

SOURCE=.\psymodel.h
# End Source File
# Begin Source File

SOURCE=.\quantize.h
# End Source File
# Begin Source File

SOURCE=.\quantize_pvt.h
# End Source File
# Begin Source File

SOURCE=.\reservoir.h
# End Source File
# Begin Source File

SOURCE=.\set_get.h
# End Source File
# Begin Source File

SOURCE=.\tables.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=.\vbrquantize.h
# End Source File
# Begin Source File

SOURCE=.\VbrTag.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# End Group
# Begin Group "Asm"

# PROP Default_Filter ".nas"
# Begin Source File

SOURCE=.\i386\choose_table.nas

!IF  "$(CFG)" == "libmp3lame - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Release NASM"

# Begin Custom Build - Assembling $(InputName)...
InputDir=.\i386
IntDir=.\..\obj\Release_NASM\libmp3lame
InputPath=.\i386\choose_table.nas
InputName=choose_table

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f win32 -i $(InputDir)/ -DWIN32 $(InputPath) -o   $(IntDir)/$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\cpu_feat.nas

!IF  "$(CFG)" == "libmp3lame - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Release NASM"

# Begin Custom Build - Assembling $(InputName)...
InputDir=.\i386
IntDir=.\..\obj\Release_NASM\libmp3lame
InputPath=.\i386\cpu_feat.nas
InputName=cpu_feat

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f win32 -i $(InputDir)/ -DWIN32 $(InputPath) -o   $(IntDir)/$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\fft.nas

!IF  "$(CFG)" == "libmp3lame - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\fft3dn.nas

!IF  "$(CFG)" == "libmp3lame - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Release NASM"

# Begin Custom Build - Assembling $(InputName)...
InputDir=.\i386
IntDir=.\..\obj\Release_NASM\libmp3lame
InputPath=.\i386\fft3dn.nas
InputName=fft3dn

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f win32 -i $(InputDir)/ -DWIN32 $(InputPath) -o   $(IntDir)/$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\fftfpu.nas

!IF  "$(CFG)" == "libmp3lame - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\fftsse.nas

!IF  "$(CFG)" == "libmp3lame - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Release NASM"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling $(InputName)...
InputDir=.\i386
IntDir=.\..\obj\Release_NASM\libmp3lame
InputPath=.\i386\fftsse.nas
InputName=fftsse

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f win32 -i $(InputDir)/ -DWIN32 $(InputPath) -o   $(IntDir)/$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\ffttbl.nas

!IF  "$(CFG)" == "libmp3lame - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386\scalar.nas

!IF  "$(CFG)" == "libmp3lame - Win32 Release"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Debug"

!ELSEIF  "$(CFG)" == "libmp3lame - Win32 Release NASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
