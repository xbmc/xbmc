# Microsoft Developer Studio Project File - Name="ftgl_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ftgl_dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ftgl_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ftgl_dll.mak" CFG="ftgl_dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ftgl_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ftgl_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ftgl_dll - Win32 Release MT" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ftgl_dll - Win32 Debug MT" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ftgl_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_ST"
# PROP BASE Intermediate_Dir "Release_ST"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Build"
# PROP Intermediate_Dir "Release_ST"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "../Build"
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FTGL_DLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "D:\cots\freetype-2.0.5\include" /I "..\..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FTGL_LIBRARY" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib glu32.lib freetype204.lib /nologo /dll /machine:I386 /out:"../Build/ftgl_dynamic_MT.dll" /libpath:"D:\cots\freetype-2.0.5\objs"

!ELSEIF  "$(CFG)" == "ftgl_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_ST"
# PROP BASE Intermediate_Dir "Debug_ST"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../Build"
# PROP Intermediate_Dir "Debug_ST"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "../Build"
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FTGL_DLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "D:\cots\freetype-2.0.5\include" /I "..\..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FTGL_LIBRARY" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib glu32.lib freetype204_D.lib /nologo /dll /pdb:"Debug_ST/ftgl_dynamic_ST_d.pdb" /debug /machine:I386 /out:"../Build/ftgl_dynamic_MT_d.dll" /pdbtype:sept /libpath:"D:\cots\freetype-2.0.5\objs"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ftgl_dll - Win32 Release MT"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_MT"
# PROP BASE Intermediate_Dir "Release_MT"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Build"
# PROP Intermediate_Dir "Release_MT"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "../Build"
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FTGL_DLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "D:\cots\freetype-2.0.5\include" /I "..\..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FTGL_LIBRARY" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib glu32.lib freetype204MT.lib /nologo /dll /machine:I386 /out:"../Build/ftgl_dynamic_MTD.dll" /libpath:"D:\cots\freetype-2.0.5\objs"

!ELSEIF  "$(CFG)" == "ftgl_dll - Win32 Debug MT"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_MT"
# PROP BASE Intermediate_Dir "Debug_MT"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../Build"
# PROP Intermediate_Dir "Debug_MT"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "../Build"
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FTGL_DLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "D:\cots\freetype-2.0.5\include" /I "..\..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FTGL_LIBRARY" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib glu32.lib freetype204MT_D.lib /nologo /dll /pdb:"Debug_ST/ftgl_dynamic_MT_d.pdb" /debug /machine:I386 /out:"../Build/ftgl_dynamic_MTD_d.dll" /pdbtype:sept /libpath:"D:\cots\freetype-2.0.5\objs"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ftgl_dll - Win32 Release"
# Name "ftgl_dll - Win32 Debug"
# Name "ftgl_dll - Win32 Release MT"
# Name "ftgl_dll - Win32 Debug MT"
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
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
