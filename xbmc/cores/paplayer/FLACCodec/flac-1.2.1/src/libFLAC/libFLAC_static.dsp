# Microsoft Developer Studio Project File - Name="libFLAC_static" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libFLAC_static - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libFLAC_static.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libFLAC_static.mak" CFG="libFLAC_static - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libFLAC_static - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libFLAC_static - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "libFLAC"
# PROP Scc_LocalPath "..\.."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libFLAC_static - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\obj\release\lib"
# PROP Intermediate_Dir "Release_static"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /Op /I ".\include" /I "..\..\include" /D VERSION=\"1.2.1\" /D "FLAC__NO_DLL" /D "FLAC__HAS_OGG" /D "FLAC__CPU_IA32" /D "FLAC__HAS_NASM" /D "FLAC__USE_3DNOW" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /nodefaultlib

!ELSEIF  "$(CFG)" == "libFLAC_static - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\obj\debug\lib"
# PROP Intermediate_Dir "Debug_static"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I ".\include" /I "..\..\include" /D VERSION=\"1.2.1\" /D "FLAC__NO_DLL" /D "FLAC__HAS_OGG" /D "FLAC__CPU_IA32" /D "FLAC__HAS_NASM" /D "FLAC__USE_3DNOW" /D "WIN32" /D "_DEBUG" /D "DEBUG" /D "FLAC__OVERFLOW_DETECT" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /nodefaultlib

!ENDIF 

# Begin Target

# Name "libFLAC_static - Win32 Release"
# Name "libFLAC_static - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "c"
# Begin Group "Assembly Files (ia32)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ia32\bitreader_asm.nasm

!IF  "$(CFG)" == "libFLAC_static - Win32 Release"

USERDEP__CPU_A="ia32/bitreader_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\bitreader_asm.nasm

"ia32/bitreader_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/bitreader_asm.nasm -o ia32/bitreader_asm.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "libFLAC_static - Win32 Debug"

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

!IF  "$(CFG)" == "libFLAC_static - Win32 Release"

USERDEP__CPU_A="ia32/cpu_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\cpu_asm.nasm

"ia32/cpu_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/cpu_asm.nasm -o ia32/cpu_asm.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "libFLAC_static - Win32 Debug"

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

!IF  "$(CFG)" == "libFLAC_static - Win32 Release"

USERDEP__FIXED="ia32/fixed_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\fixed_asm.nasm

"ia32/fixed_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/fixed_asm.nasm -o ia32/fixed_asm.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "libFLAC_static - Win32 Debug"

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

!IF  "$(CFG)" == "libFLAC_static - Win32 Release"

USERDEP__LPC_A="ia32/lpc_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\lpc_asm.nasm

"ia32/lpc_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/lpc_asm.nasm -o ia32/lpc_asm.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "libFLAC_static - Win32 Debug"

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

!IF  "$(CFG)" == "libFLAC_static - Win32 Release"

USERDEP__CPU_A="ia32/stream_encoder_asm.nasm"	
# Begin Custom Build
InputPath=.\ia32\stream_encoder_asm.nasm

"ia32/stream_encoder_asm.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ia32/ ia32/stream_encoder_asm.nasm -o ia32/stream_encoder_asm.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "libFLAC_static - Win32 Debug"

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
