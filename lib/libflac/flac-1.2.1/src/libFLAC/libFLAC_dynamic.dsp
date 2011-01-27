# Microsoft Developer Studio Project File - Name="libFLAC_dynamic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libFLAC_dynamic - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libFLAC_dynamic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libFLAC_dynamic.mak" CFG="libFLAC_dynamic - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libFLAC_dynamic - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libFLAC_dynamic - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "libFLAC"
# PROP Scc_LocalPath "..\.."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libFLAC_dynamic - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\obj\release\lib"
# PROP Intermediate_Dir "Release_dynamic"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I ".\include" /I "..\..\include" /D "NDEBUG" /D "FLAC_API_EXPORTS" /D "FLAC__HAS_OGG" /D VERSION=\"1.2.1\" /D "FLAC__CPU_IA32" /D "FLAC__HAS_NASM" /D "FLAC__USE_3DNOW" /D "_WINDOWS" /D "_WINDLL" /D "WIN32" /D "_USRDLL" /FR /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_USRDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\..\obj\release\lib\ogg_static.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\..\obj\release\bin/libFLAC.dll"

!ELSEIF  "$(CFG)" == "libFLAC_dynamic - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\obj\debug\lib"
# PROP Intermediate_Dir "Debug_dynamic"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I ".\include" /I "..\..\include" /D "_DEBUG" /D "DEBUG" /D "FLAC__OVERFLOW_DETECT" /D "FLAC_API_EXPORTS" /D "FLAC__HAS_OGG" /D VERSION=\"1.2.1\" /D "FLAC__CPU_IA32" /D "FLAC__HAS_NASM" /D "FLAC__USE_3DNOW" /D "_WINDOWS" /D "_WINDLL" /D "WIN32" /D "_USRDLL" /FR /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_USRDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\obj\release\lib\ogg_static.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\..\obj\debug\bin/libFLAC.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "libFLAC_dynamic - Win32 Release"
# Name "libFLAC_dynamic - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "c"
# Begin Group "Assembly Files (ia32)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ia32\bitreader_asm.nasm

!IF  "$(CFG)" == "libFLAC_dynamic - Win32 Release"

USERDEP__CPU_A="ia32/bitreader_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\bitreader_asm.nasm

"ia32/bitreader_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/bitreader_asm.nasm -o ia32/bitreader_asm.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "libFLAC_dynamic - Win32 Debug"

USERDEP__CPU_A="ia32/bitreader_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\bitreader_asm.nasm

"ia32/bitreader_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/bitreader_asm.nasm -o ia32/bitreader_asm.obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ia32\cpu_asm.nasm

!IF  "$(CFG)" == "libFLAC_dynamic - Win32 Release"

USERDEP__CPU_A="ia32/cpu_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\cpu_asm.nasm

"ia32/cpu_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/cpu_asm.nasm -o ia32/cpu_asm.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "libFLAC_dynamic - Win32 Debug"

USERDEP__CPU_A="ia32/cpu_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\cpu_asm.nasm

"ia32/cpu_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/cpu_asm.nasm -o ia32/cpu_asm.obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ia32\fixed_asm.nasm

!IF  "$(CFG)" == "libFLAC_dynamic - Win32 Release"

USERDEP__FIXED="ia32/fixed_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\fixed_asm.nasm

"ia32/fixed_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/fixed_asm.nasm -o ia32/fixed_asm.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "libFLAC_dynamic - Win32 Debug"

USERDEP__FIXED="ia32/fixed_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\fixed_asm.nasm

"ia32/fixed_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/fixed_asm.nasm -o ia32/fixed_asm.obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ia32\lpc_asm.nasm

!IF  "$(CFG)" == "libFLAC_dynamic - Win32 Release"

USERDEP__LPC_A="ia32/lpc_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\lpc_asm.nasm

"ia32/lpc_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/lpc_asm.nasm -o ia32/lpc_asm.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "libFLAC_dynamic - Win32 Debug"

USERDEP__LPC_A="ia32/lpc_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\lpc_asm.nasm

"ia32/lpc_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/lpc_asm.nasm -o ia32/lpc_asm.obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ia32\stream_encoder_asm.nasm

!IF  "$(CFG)" == "libFLAC_dynamic - Win32 Release"

USERDEP__CPU_A="ia32/stream_encoder_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\stream_encoder_asm.nasm

"ia32/stream_encoder_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/stream_encoder_asm.nasm -o ia32/stream_encoder_asm.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "libFLAC_dynamic - Win32 Debug"

USERDEP__CPU_A="ia32/stream_encoder_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\stream_encoder_asm.nasm

"ia32/stream_encoder_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/stream_encoder_asm.nasm -o ia32/stream_encoder_asm.obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ia32\nasm.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\bitmath.c
# End Source File
# Begin Source File

SOURCE=.\bitreader.c
# End Source File
# Begin Source File

SOURCE=.\bitwriter.c
# End Source File
# Begin Source File

SOURCE=.\cpu.c
# End Source File
# Begin Source File

SOURCE=.\crc.c
# End Source File
# Begin Source File

SOURCE=.\fixed.c
# End Source File
# Begin Source File

SOURCE=.\float.c
# End Source File
# Begin Source File

SOURCE=.\format.c
# End Source File
# Begin Source File

SOURCE=.\lpc.c
# End Source File
# Begin Source File

SOURCE=.\md5.c
# End Source File
# Begin Source File

SOURCE=.\memory.c
# End Source File
# Begin Source File

SOURCE=.\metadata_iterators.c
# End Source File
# Begin Source File

SOURCE=.\metadata_object.c
# End Source File
# Begin Source File

SOURCE=.\ogg_decoder_aspect.c
# End Source File
# Begin Source File

SOURCE=.\ogg_encoder_aspect.c
# End Source File
# Begin Source File

SOURCE=.\ogg_helper.c
# End Source File
# Begin Source File

SOURCE=.\ogg_mapping.c
# End Source File
# Begin Source File

SOURCE=.\stream_decoder.c
# End Source File
# Begin Source File

SOURCE=.\stream_encoder.c
# End Source File
# Begin Source File

SOURCE=.\stream_encoder_framing.c
# End Source File
# Begin Source File

SOURCE=.\window.c
# End Source File
# End Group
# Begin Group "Private Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\private\all.h
# End Source File
# Begin Source File

SOURCE=.\include\private\bitmath.h
# End Source File
# Begin Source File

SOURCE=.\include\private\bitreader.h
# End Source File
# Begin Source File

SOURCE=.\include\private\bitwriter.h
# End Source File
# Begin Source File

SOURCE=.\include\private\cpu.h
# End Source File
# Begin Source File

SOURCE=.\include\private\crc.h
# End Source File
# Begin Source File

SOURCE=.\include\private\fixed.h
# End Source File
# Begin Source File

SOURCE=.\include\private\float.h
# End Source File
# Begin Source File

SOURCE=.\include\private\format.h
# End Source File
# Begin Source File

SOURCE=.\include\private\lpc.h
# End Source File
# Begin Source File

SOURCE=.\include\private\md5.h
# End Source File
# Begin Source File

SOURCE=.\include\private\memory.h
# End Source File
# Begin Source File

SOURCE=.\include\private\metadata.h
# End Source File
# Begin Source File

SOURCE=.\include\private\ogg_decoder_aspect.h
# End Source File
# Begin Source File

SOURCE=.\include\private\ogg_encoder_aspect.h
# End Source File
# Begin Source File

SOURCE=.\include\private\ogg_helper.h
# End Source File
# Begin Source File

SOURCE=.\include\private\ogg_mapping.h
# End Source File
# Begin Source File

SOURCE=.\include\private\stream_encoder_framing.h
# End Source File
# Begin Source File

SOURCE=.\include\private\window.h
# End Source File
# End Group
# Begin Group "Protected Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\protected\all.h
# End Source File
# Begin Source File

SOURCE=.\include\protected\stream_decoder.h
# End Source File
# Begin Source File

SOURCE=.\include\protected\stream_encoder.h
# End Source File
# End Group
# Begin Group "Public Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\FLAC\all.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FLAC\assert.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FLAC\export.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FLAC\format.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FLAC\metadata.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FLAC\ordinals.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FLAC\stream_decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FLAC\stream_encoder.h
# End Source File
# End Group
# End Target
# End Project
