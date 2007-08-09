# Microsoft Developer Studio Project File - Name="ftgl_static_lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ftgl_static_lib - Win32 Debug MT
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ftgl_static_lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ftgl_static_lib.mak" CFG="ftgl_static_lib - Win32 Debug MT"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ftgl_static_lib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ftgl_static_lib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "ftgl_static_lib - Win32 Debug MT" (based on "Win32 (x86) Static Library")
!MESSAGE "ftgl_static_lib - Win32 Release MT" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ftgl_static_lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_ST"
# PROP BASE Intermediate_Dir "Release_ST"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_ST"
# PROP Intermediate_Dir "Release_ST"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "D:\cots\freetype-2.0.5\include" /I "..\..\include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "FTGL_LIBRARY_STATIC" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Build\ftgl_static_MT.lib"

!ELSEIF  "$(CFG)" == "ftgl_static_lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_ST"
# PROP BASE Intermediate_Dir "Debug_ST"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_ST"
# PROP Intermediate_Dir "Debug_ST"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "D:\cots\freetype-2.0.5\include" /I "..\..\include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "FTGL_LIBRARY_STATIC" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Build\ftgl_static_MT_d.lib"

!ELSEIF  "$(CFG)" == "ftgl_static_lib - Win32 Debug MT"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_MT"
# PROP BASE Intermediate_Dir "Debug_MT"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_MT"
# PROP Intermediate_Dir "Debug_MT"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "D:\cots\freetype-2.0.5\include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "FTGL_LIBRARY_STATIC" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "D:\cots\freetype-2.0.5\include" /I "..\..\include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "FTGL_LIBRARY_STATIC" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Build\ftgl_static_MTD_d.lib"

!ELSEIF  "$(CFG)" == "ftgl_static_lib - Win32 Release MT"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_MT"
# PROP BASE Intermediate_Dir "Release_MT"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_MT"
# PROP Intermediate_Dir "Release_MT"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "FTGL_LIBRARY_STATIC" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "D:\cots\freetype-2.0.5\include" /I "..\..\include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "FTGL_LIBRARY_STATIC" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Build\ftgl_static_MTD.lib"

!ENDIF 

# Begin Target

# Name "ftgl_static_lib - Win32 Release"
# Name "ftgl_static_lib - Win32 Debug"
# Name "ftgl_static_lib - Win32 Debug MT"
# Name "ftgl_static_lib - Win32 Release MT"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\FTBitmapGlyph.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTCharmap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTContour.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTExtrdGlyph.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTFace.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTFont.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTGLBitmapFont.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTGLExtrdFont.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTGLOutlineFont.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTGLPixmapFont.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTGLPolygonFont.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTGLTextureFont.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTGlyph.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTGlyphContainer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTLibrary.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTOutlineGlyph.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTPixmapGlyph.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTPoint.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTPolyGlyph.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTSize.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTTextureGlyph.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FTVectoriser.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\FTBBox.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTBitmapGlyph.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTCharmap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTCharToGlyphIndexMap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTContour.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTExtrdGlyph.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTFace.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTFont.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTGL.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTGLBitmapFont.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTGLExtrdFont.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTGLOutlineFont.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTGLPixmapFont.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTGLPolygonFont.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTGLTextureFont.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTGlyph.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTGlyphContainer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTLibrary.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTOutlineGlyph.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTPixmapGlyph.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTPoint.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTPolyGlyph.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTSize.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTTextureGlyph.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTVector.h
# End Source File
# Begin Source File

SOURCE=..\..\include\FTVectoriser.h
# End Source File
# End Group
# End Target
# End Project
