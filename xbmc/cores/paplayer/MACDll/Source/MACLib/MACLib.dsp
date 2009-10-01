# Microsoft Developer Studio Project File - Name="MACLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=MACLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MACLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MACLib.mak" CFG="MACLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MACLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "MACLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Monkey's Audio/MACLib", SCAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MACLib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /GX /Ox /Ot /Og /Oi /Ob2 /I "..\Shared" /D "WIN32" /D "NDEBUG" /D "_LIB" /D "_UNICODE" /D "UNICODE" /FR /YX"all.h" /FD /c
# SUBTRACT CPP /Oa
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Special Build Tool
ProjDir=.
SOURCE="$(InputPath)"
PreLink_Desc=Building assembly...
PreLink_Cmds=cd $(ProjDir)\Assembly	nasmw -d WIN32 -f win32 -o Assembly.obj Assembly.nas
# End Special Build Tool

!ELSEIF  "$(CFG)" == "MACLib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\Shared" /D "WIN32" /D "_DEBUG" /D "_LIB" /D "_UNICODE" /D "UNICODE" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Special Build Tool
ProjDir=.
SOURCE="$(InputPath)"
PreLink_Desc=Building assembly...
PreLink_Cmds=cd $(ProjDir)\Assembly	nasmw -d WIN32 -f win32 -o Assembly.obj Assembly.nas
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "MACLib - Win32 Release"
# Name "MACLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Compress"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\APECompress.cpp
# End Source File
# Begin Source File

SOURCE=.\APECompressCore.cpp
# End Source File
# Begin Source File

SOURCE=.\APECompressCreate.cpp
# End Source File
# Begin Source File

SOURCE=.\BitArray.cpp
# End Source File
# End Group
# Begin Group "Decompress"

# PROP Default_Filter ""
# Begin Group "Old"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\Old\Anti-Predictor.cpp"
# End Source File
# Begin Source File

SOURCE=.\Old\AntiPredictorExtraHigh.cpp
# End Source File
# Begin Source File

SOURCE=.\Old\AntiPredictorFast.cpp
# End Source File
# Begin Source File

SOURCE=.\Old\AntiPredictorHigh.cpp
# End Source File
# Begin Source File

SOURCE=.\Old\AntiPredictorNormal.cpp
# End Source File
# Begin Source File

SOURCE=.\Old\APEDecompressCore.cpp
# End Source File
# Begin Source File

SOURCE=.\Old\APEDecompressOld.cpp
# End Source File
# Begin Source File

SOURCE=.\Old\UnBitArrayOld.cpp
# End Source File
# Begin Source File

SOURCE=.\Old\UnMAC.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\APEDecompress.cpp
# End Source File
# Begin Source File

SOURCE=.\UnBitArray.cpp
# End Source File
# Begin Source File

SOURCE=.\UnBitArrayBase.cpp
# End Source File
# End Group
# Begin Group "Info"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\APEHeader.cpp
# End Source File
# Begin Source File

SOURCE=.\APEInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\APELink.cpp
# End Source File
# Begin Source File

SOURCE=.\APETag.cpp
# End Source File
# Begin Source File

SOURCE=.\WAVInputSource.cpp
# End Source File
# End Group
# Begin Group "Tools"

# PROP Default_Filter ""
# Begin Group "IO"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Shared\StdLibFileIO.cpp
# End Source File
# Begin Source File

SOURCE=..\Shared\WinFileIO.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\Shared\CharacterHelper.cpp
# End Source File
# Begin Source File

SOURCE=..\Shared\CircleBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\Shared\GlobalFunctions.cpp
# End Source File
# Begin Source File

SOURCE=.\MACProgressHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\MD5.cpp
# End Source File
# Begin Source File

SOURCE=.\Prepare.cpp
# End Source File
# End Group
# Begin Group "Prediction"

# PROP Default_Filter ""
# Begin Group "Filters"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\NNFilter.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\NewPredictor.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\APESimple.cpp
# End Source File
# Begin Source File

SOURCE=.\MACLib.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "Compress (h)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\APECompress.h
# End Source File
# Begin Source File

SOURCE=.\APECompressCore.h
# End Source File
# Begin Source File

SOURCE=.\APECompressCreate.h
# End Source File
# Begin Source File

SOURCE=.\BitArray.h
# End Source File
# End Group
# Begin Group "Decompress (h)"

# PROP Default_Filter ""
# Begin Group "Old (h)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\Old\Anti-Predictor.h"
# End Source File
# Begin Source File

SOURCE=.\Old\APEDecompressCore.h
# End Source File
# Begin Source File

SOURCE=.\Old\APEDecompressOld.h
# End Source File
# Begin Source File

SOURCE=.\Old\UnBitArrayOld.h
# End Source File
# Begin Source File

SOURCE=.\Old\UnMAC.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\APEDecompress.h
# End Source File
# Begin Source File

SOURCE=.\UnBitArray.h
# End Source File
# Begin Source File

SOURCE=.\UnBitArrayBase.h
# End Source File
# End Group
# Begin Group "Info (h)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\APEHeader.h
# End Source File
# Begin Source File

SOURCE=.\APEInfo.h
# End Source File
# Begin Source File

SOURCE=.\APELink.h
# End Source File
# Begin Source File

SOURCE=.\APETag.h
# End Source File
# Begin Source File

SOURCE=.\WAVInputSource.h
# End Source File
# End Group
# Begin Group "Tools (h)"

# PROP Default_Filter ""
# Begin Group "IO (h)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Shared\IO.h
# End Source File
# Begin Source File

SOURCE=..\Shared\StdLibFileIO.h
# End Source File
# Begin Source File

SOURCE=..\Shared\WinFileIO.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\Shared\CharacterHelper.h
# End Source File
# Begin Source File

SOURCE=..\Shared\CircleBuffer.h
# End Source File
# Begin Source File

SOURCE=..\Shared\GlobalFunctions.h
# End Source File
# Begin Source File

SOURCE=.\MACProgressHelper.h
# End Source File
# Begin Source File

SOURCE=.\md5.h
# End Source File
# Begin Source File

SOURCE=..\Shared\NoWindows.h
# End Source File
# Begin Source File

SOURCE=.\Prepare.h
# End Source File
# Begin Source File

SOURCE=..\Shared\SmartPtr.h
# End Source File
# End Group
# Begin Group "Prediction (h)"

# PROP Default_Filter ""
# Begin Group "Filters (h)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\NNFilter.h
# End Source File
# Begin Source File

SOURCE=..\Shared\RollBuffer.h
# End Source File
# Begin Source File

SOURCE=.\ScaledFirstOrderFilter.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\NewPredictor.h
# End Source File
# Begin Source File

SOURCE=.\Predictor.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\Shared\All.h
# End Source File
# Begin Source File

SOURCE=.\Assembly\Assembly.h
# End Source File
# Begin Source File

SOURCE=.\MACLib.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\Credits.txt
# End Source File
# Begin Source File

SOURCE=..\History.txt
# End Source File
# Begin Source File

SOURCE="..\To Do.txt"
# End Source File
# Begin Source File

SOURCE=.\Assembly\Assembly.obj
# End Source File
# End Target
# End Project
