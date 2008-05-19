# Microsoft Developer Studio Project File - Name="7z_C" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=7z_C - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "7z_C.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "7z_C.mak" CFG="7z_C - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "7z_C - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "7z_C - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "7z_C - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_LZMA_PROB32" /D "_LZMA_IN_CB" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"Release/7zDec.exe" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "7z_C - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W4 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_LZMA_PROB32" /D "_LZMA_IN_CB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/7zDec.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "7z_C - Win32 Release"
# Name "7z_C - Win32 Debug"
# Begin Group "LZMA"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Lzma\LzmaDecode.c
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Lzma\LzmaDecode.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Lzma\LzmaTypes.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\7zCrc.c
# End Source File
# Begin Source File

SOURCE=..\..\7zCrc.h
# End Source File
# Begin Source File

SOURCE=..\..\Types.h
# End Source File
# End Group
# Begin Group "Branch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Branch\BranchTypes.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\BranchX86.c
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\BranchX86.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\BranchX86_2.c
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\BranchX86_2.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\7zAlloc.c
# End Source File
# Begin Source File

SOURCE=.\7zAlloc.h
# End Source File
# Begin Source File

SOURCE=.\7zBuffer.c
# End Source File
# Begin Source File

SOURCE=.\7zBuffer.h
# End Source File
# Begin Source File

SOURCE=.\7zDecode.c
# End Source File
# Begin Source File

SOURCE=.\7zDecode.h
# End Source File
# Begin Source File

SOURCE=.\7zExtract.c
# End Source File
# Begin Source File

SOURCE=.\7zExtract.h
# End Source File
# Begin Source File

SOURCE=.\7zHeader.c
# End Source File
# Begin Source File

SOURCE=.\7zHeader.h
# End Source File
# Begin Source File

SOURCE=.\7zIn.c
# End Source File
# Begin Source File

SOURCE=.\7zIn.h
# End Source File
# Begin Source File

SOURCE=.\7zItem.c
# End Source File
# Begin Source File

SOURCE=.\7zItem.h
# End Source File
# Begin Source File

SOURCE=.\7zMain.c
# End Source File
# Begin Source File

SOURCE=.\7zMethodID.c
# End Source File
# Begin Source File

SOURCE=.\7zMethodID.h
# End Source File
# End Target
# End Project
