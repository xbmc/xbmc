# Microsoft Developer Studio Project File - Name="mpglib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=mpglib - Win32 Release
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "mpglib_vc6.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "mpglib_vc6.mak" CFG="mpglib - Win32 Release"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "mpglib - Win32 Release" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE "mpglib - Win32 Debug" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE "mpglib - Win32 Release NASM" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 1
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mpglib - Win32 Release"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\output\Release"
# PROP Intermediate_Dir "..\obj\Release\mpglib"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "HAVE_CONFIG_H" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /O2 /Ob2 /I "../libmp3lame" /I "../include" /I ".." /D "NDEBUG" /D "HAVE_MPGLIB" /D "_WINDOWS" /D "USE_LAYER_2" /D "WIN32" /D "HAVE_CONFIG_H" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\output\Release\mpglib-static.lib"

!ELSEIF  "$(CFG)" == "mpglib - Win32 Debug"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\output\Debug"
# PROP Intermediate_Dir "..\obj\Debug\mpglib"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "HAVE_CONFIG_H" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /ZI /Od /I "../libmp3lame" /I "../include" /I ".." /D "_DEBUG" /D "_WINDOWS" /D "USE_LAYER_2" /D "HAVE_MPGLIB" /D "WIN32" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\output\Debug\mpglib-static.lib"

!ELSEIF  "$(CFG)" == "mpglib - Win32 Release NASM"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "mpglib___Win32_Release_NASM"
# PROP BASE Intermediate_Dir "mpglib___Win32_Release_NASM"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\output\Release_NASM"
# PROP Intermediate_Dir "..\obj\Release_NASM\mpglib"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /O2 /Ob2 /I "../libmp3lame" /I "../include" /I ".." /D "NDEBUG" /D "HAVE_MPGLIB" /D "_WINDOWS" /D "USE_LAYER_2" /D "USE_LAYER_1" /D "WIN32" /D "HAVE_CONFIG_H" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /W3 /O2 /Ob2 /I "../libmp3lame" /I "../include" /I ".." /D "NDEBUG" /D "HAVE_MPGLIB" /D "_WINDOWS" /D "USE_LAYER_2" /D "WIN32" /D "HAVE_CONFIG_H" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"Release\mpglib.lib"
# ADD LIB32 /nologo /out:"..\output\Release_NASM\mpglib-static.lib"

!ENDIF 

# Begin Target

# Name "mpglib - Win32 Release"
# Name "mpglib - Win32 Debug"
# Name "mpglib - Win32 Release NASM"
# Begin Group "Source"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=.\common.c
# End Source File
# Begin Source File

SOURCE=.\dct64_i386.c
# End Source File
# Begin Source File

SOURCE=.\decode_i386.c
# End Source File
# Begin Source File

SOURCE=.\interface.c
# End Source File
# Begin Source File

SOURCE=.\layer1.c
# End Source File
# Begin Source File

SOURCE=.\layer2.c
# End Source File
# Begin Source File

SOURCE=.\layer3.c
# End Source File
# Begin Source File

SOURCE=.\tabinit.c
# End Source File
# End Group
# Begin Group "Include"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=..\configMS.h

!IF  "$(CFG)" == "mpglib - Win32 Release"

# Begin Custom Build - Performing Custom Build Step on $(InputName)
InputPath=..\configMS.h

"..\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy ..\configMS.h ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "mpglib - Win32 Debug"

# Begin Custom Build - Performing Custom Build Step on $(InputName)
InputPath=..\configMS.h

"..\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy ..\configMS.h ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "mpglib - Win32 Release NASM"

# Begin Custom Build - Performing Custom Build Step on $(InputName)
InputPath=..\configMS.h

"..\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy ..\configMS.h ..\config.h

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dct64_i386.h
# End Source File
# Begin Source File

SOURCE=.\decode_i386.h
# End Source File
# Begin Source File

SOURCE=.\huffman.h
# End Source File
# Begin Source File

SOURCE=.\interface.h
# End Source File
# Begin Source File

SOURCE=.\layer1.h
# End Source File
# Begin Source File

SOURCE=.\layer2.h
# End Source File
# Begin Source File

SOURCE=.\layer3.h
# End Source File
# Begin Source File

SOURCE=.\mpg123.h
# End Source File
# Begin Source File

SOURCE=.\mpglib.h
# End Source File
# Begin Source File

SOURCE=.\tabinit.h
# End Source File
# End Group
# End Target
# End Project
