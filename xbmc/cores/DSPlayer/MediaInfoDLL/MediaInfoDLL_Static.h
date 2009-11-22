/* MediaInfoDLL - All info about media files, for DLL
// Copyright (C) 2002-2009 Jerome Martinez, Zen@MediaArea.net
//
// This library is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library. If not, see <http://www.gnu.org/licenses/>.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Public DLL interface implementation
// Wrapper for MediaInfo Library
// Please see MediaInfo.h for help
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef MediaInfoDLL_StaticH
#define MediaInfoDLL_StaticH

//***************************************************************************
// Platforms (from libzen)
//***************************************************************************

/*---------------------------------------------------------------------------*/
/*Win32*/
#if defined(__NT__) || defined(_WIN32) || defined(WIN32)
    #ifndef WIN32
        #define WIN32
    #endif
    #ifndef _WIN32
        #define _WIN32
    #endif
    #ifndef __WIN32__
        #define __WIN32__ 1
    #endif
#endif

/*---------------------------------------------------------------------------*/
/*Win64*/
#if defined(_WIN64) || defined(WIN64)
    #ifndef WIN64
        #define WIN64
    #endif
    #ifndef _WIN64
        #define _WIN64
    #endif
    #ifndef __WIN64__
        #define __WIN64__ 1
    #endif
#endif

/*---------------------------------------------------------------------------*/
/*Windows*/
#if defined(WIN32) || defined(WIN64)
    #ifndef WINDOWS
        #define WINDOWS
    #endif
    #ifndef _WINDOWS
        #define _WINDOWS
    #endif
    #ifndef __WINDOWS__
        #define __WINDOWS__ 1
    #endif
#endif

/*---------------------------------------------------------------------------*/
/*Unix (Linux, HP, Sun, BeOS...)*/
#if defined(UNIX) || defined(_UNIX) || defined(__UNIX__) \
    || defined(__unix) || defined(__unix__) \
    || defined(____SVR4____) || defined(__LINUX__) || defined(__sgi) \
    || defined(__hpux) || defined(sun) || defined(__SUN__) || defined(_AIX) \
    || defined(__EMX__) || defined(__VMS) || defined(__BEOS__)
    #ifndef UNIX
        #define UNIX
    #endif
    #ifndef _UNIX
        #define _UNIX
    #endif
    #ifndef __UNIX__
        #define __UNIX__ 1
    #endif
#endif

/*---------------------------------------------------------------------------*/
/*MacOS Classic*/
#if defined(macintosh)
    #ifndef MACOS
        #define MACOS
    #endif
    #ifndef _MACOS
        #define _MACOS
    #endif
    #ifndef __MACOS__
        #define __MACOS__ 1
    #endif
#endif

/*---------------------------------------------------------------------------*/
/*MacOS X*/
#if defined(__APPLE__) && defined(__MACH__)
    #ifndef MACOSX
        #define MACOSX
    #endif
    #ifndef _MACOSX
        #define _MACOSX
    #endif
    #ifndef __MACOSX__
        #define __MACOSX__ 1
    #endif
#endif

/*Test of targets*/
#if defined(WINDOWS) && defined(UNIX) && defined(MACOS) && defined(MACOSX)
    #pragma message Multiple platforms???
#endif

#if !defined(WIN32) && !defined(UNIX) && !defined(MACOS) && !defined(MACOSX)
    #pragma message No known platforms, assume default
#endif

/*-------------------------------------------------------------------------*/
#if defined(_WIN32) && !defined(__MINGW32__) //MinGW32 does not support _declspec
    #ifdef MEDIAINFO_DLL_EXPORT
        #define MEDIAINFO_EXP extern _declspec(dllexport)
    #else
        #define MEDIAINFO_EXP extern _declspec(dllimport)
    #endif
#else //defined(_WIN32) && !defined(__MINGW32__)
    #define MEDIAINFO_EXP
#endif //defined(_WIN32) && !defined(__MINGW32__)

#if !defined(_WIN32) && !defined(__WIN32__)
    #define __stdcall
#endif //!defined(_WIN32)
#include <limits.h>

/*-------------------------------------------------------------------------*/
/*8-bit int                                                                */
#if UCHAR_MAX==0xff
    #undef  MAXTYPE_INT
    #define MAXTYPE_INT 8
    typedef unsigned char       MediaInfo_int8u;
#else
    #pragma message This machine has no 8-bit integertype?
#endif
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/*64-bit int                                                               */
#if defined(__MINGW32__) || defined(__CYGWIN32__) || defined(__UNIX__) || defined(__MACOSX__)
    #undef  MAXTYPE_INT
    #define MAXTYPE_INT 64
    typedef unsigned long long  MediaInfo_int64u;
#elif defined(__WIN32__) ||  defined(_WIN32)
    #undef  MAXTYPE_INT
    #define MAXTYPE_INT 64
    typedef unsigned __int64    MediaInfo_int64u;
#else
    #pragma message This machine has no 64-bit integer type?
#endif
/*-------------------------------------------------------------------------*/

/** @brief Kinds of Stream */
typedef enum MediaInfo_stream_t
{
    MediaInfo_Stream_General,
    MediaInfo_Stream_Video,
    MediaInfo_Stream_Audio,
    MediaInfo_Stream_Text,
    MediaInfo_Stream_Chapters,
    MediaInfo_Stream_Image,
    MediaInfo_Stream_Menu,
    MediaInfo_Stream_Max
} MediaInfo_stream_C;

/** @brief Kinds of Info */
typedef enum MediaInfo_info_t
{
    MediaInfo_Info_Name,
    MediaInfo_Info_Text,
    MediaInfo_Info_Measure,
    MediaInfo_Info_Options,
    MediaInfo_Info_Name_Text,
    MediaInfo_Info_Measure_Text,
    MediaInfo_Info_Info,
    MediaInfo_Info_HowTo,
    MediaInfo_Info_Max
} MediaInfo_info_C;

/** @brief Option if InfoKind = Info_Options */
typedef enum MediaInfo_infooptions_t
{
    MediaInfo_InfoOption_ShowInInform,
    MediaInfo_InfoOption_Reserved,
    MediaInfo_InfoOption_ShowInSupported,
    MediaInfo_InfoOption_TypeOfValue,
    MediaInfo_InfoOption_Max
} MediaInfo_infooptions_C;

/** @brief File opening options */
typedef enum MediaInfo_fileoptions_t
{
    MediaInfo_FileOption_Nothing        =0x00,
    MediaInfo_FileOption_NoRecursive    =0x01,
    MediaInfo_FileOption_CloseAll       =0x02,
    MediaInfo_FileOption_Max            =0x04
} MediaInfo_fileoptions_C;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/***************************************************************************/
/*! \file MediaInfoDll.h
\brief DLL wrapper for MediaInfo.h.

DLL wrapper for MediaInfo.h \n
    Can be used for C and C++\n
    "Handle" replaces class definition
*/
/***************************************************************************/

#if defined (MEDIAINFO_DLL_EXPORT) || (defined (UNICODE) || defined (_UNICODE)) //DLL construction or Unicode
/** @brief A 'new' MediaInfo interface, return a Handle, don't forget to delete it after using it*/
MEDIAINFO_EXP void*             __stdcall MediaInfo_New (); /*you must ALWAYS call MediaInfo_Delete(Handle) in order to free memory*/
/** @brief A 'new' MediaInfo interface (with a quick init of useful options : "**VERSION**;**APP_NAME**;**APP_VERSION**", but without debug information, use it only if you know what you do), return a Handle, don't forget to delete it after using it*/
MEDIAINFO_EXP void*             __stdcall MediaInfo_New_Quick (const wchar_t* File, const wchar_t* Options); /*you must ALWAYS call MediaInfo_Delete(Handle) in order to free memory*/
/** @brief Delete a MediaInfo interface*/
MEDIAINFO_EXP void              __stdcall MediaInfo_Delete (void* Handle);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a filename)*/
MEDIAINFO_EXP size_t            __stdcall MediaInfo_Open (void* Handle, const wchar_t* File);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a buffer) */
MEDIAINFO_EXP size_t            __stdcall MediaInfo_Open_Buffer (void* Handle, const unsigned char* Begin, size_t Begin_Size, const unsigned char* End, size_t End_Size); /*return Handle*/
/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a buffer, Init) */
MEDIAINFO_EXP size_t            __stdcall MediaInfo_Open_Buffer_Init (void* Handle, MediaInfo_int64u File_Size, MediaInfo_int64u File_Offset);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a buffer, Continue) */
MEDIAINFO_EXP size_t            __stdcall MediaInfo_Open_Buffer_Continue (void* Handle, MediaInfo_int8u* Buffer, size_t Buffer_Size);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a buffer, Continue_GoTo_Get) */
MEDIAINFO_EXP MediaInfo_int64u  __stdcall MediaInfo_Open_Buffer_Continue_GoTo_Get (void* Handle);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a buffer, Finalize) */
MEDIAINFO_EXP size_t            __stdcall MediaInfo_Open_Buffer_Finalize (void* Handle);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Save */
MEDIAINFO_EXP size_t            __stdcall MediaInfo_Save (void* Handle);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Close */
MEDIAINFO_EXP void              __stdcall MediaInfo_Close (void* Handle);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Inform */
MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfo_Inform (void* Handle, size_t Reserved); /*Default : Reserved=0*/
/** @brief Wrapper for MediaInfoLib::MediaInfo::Get */
MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfo_GetI (void* Handle, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, MediaInfo_info_C InfoKind); /*Default : InfoKind=Info_Text*/
/** @brief Wrapper for MediaInfoLib::MediaInfo::Get */
MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfo_Get (void* Handle, MediaInfo_stream_C StreamKind, size_t StreamNumber, const wchar_t* Parameter, MediaInfo_info_C InfoKind, MediaInfo_info_C SearchKind); /*Default : InfoKind=Info_Text, SearchKind=Info_Name*/
/** @brief Wrapper for MediaInfoLib::MediaInfo::Set */
MEDIAINFO_EXP size_t            __stdcall MediaInfo_SetI (void* Handle, const wchar_t* ToSet, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, const wchar_t* OldParameter);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Set */
MEDIAINFO_EXP size_t            __stdcall MediaInfo_Set (void* Handle, const wchar_t* ToSet, MediaInfo_stream_C StreamKind, size_t StreamNumber, const wchar_t* Parameter, const wchar_t* OldParameter);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Option */
MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfo_Option (void* Handle, const wchar_t* Option, const wchar_t* Value);
/** @brief Wrapper for MediaInfoLib::MediaInfo::State_Get */
MEDIAINFO_EXP size_t            __stdcall MediaInfo_State_Get (void* Handle);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Count_Get */
MEDIAINFO_EXP size_t            __stdcall MediaInfo_Count_Get (void* Handle, MediaInfo_stream_C StreamKind, size_t StreamNumber); /*Default : StreamNumber=-1*/
#else //defined (MEDIAINFO_DLL_EXPORT) || (defined (UNICODE) || defined (_UNICODE))
    #define MediaInfo_New               MediaInfoA_New
    #define MediaInfo_New_Quick         MediaInfoA_New_Quick
    #define MediaInfo_Delete            MediaInfoA_Delete
    #define MediaInfo_Open              MediaInfoA_Open
    #define MediaInfo_Open_Buffer       MediaInfoA_Open_Buffer
    #define MediaInfo_Save              MediaInfoA_Save
    #define MediaInfo_Close             MediaInfoA_Close
    #define MediaInfo_Inform            MediaInfoA_Inform
    #define MediaInfo_GetI              MediaInfoA_GetI
    #define MediaInfo_Get               MediaInfoA_Get
    #define MediaInfo_SetI              MediaInfoA_SetI
    #define MediaInfo_Set               MediaInfoA_Set
    #define MediaInfo_Option            MediaInfoA_Option
    #define MediaInfo_State_Get         MediaInfoA_State_Get
    #define MediaInfo_Count_Get         MediaInfoA_Count_Get
#endif //defined (MEDIAINFO_DLL_EXPORT) || (defined (UNICODE) || defined (_UNICODE))

/** @brief A 'new' MediaInfo interface, return a Handle, don't forget to delete it after using it*/
MEDIAINFO_EXP void*             __stdcall MediaInfoA_New (); /*you must ALWAYS call MediaInfo_Delete(Handle) in order to free memory*/
/** @brief A 'new' MediaInfo interface (with a quick init of useful options : "**VERSION**;**APP_NAME**;**APP_VERSION**", but without debug information, use it only if you know what you do), return a Handle, don't forget to delete it after using it*/
MEDIAINFO_EXP void*             __stdcall MediaInfoA_New_Quick (const char* File, const char* Options); /*you must ALWAYS call MediaInfo_Delete(Handle) in order to free memory*/
/** @brief Delete a MediaInfo interface*/
MEDIAINFO_EXP void              __stdcall MediaInfoA_Delete (void* Handle);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a filename)*/
MEDIAINFO_EXP size_t            __stdcall MediaInfoA_Open (void* Handle, const char* File); /*you must ALWAYS call MediaInfo_Close(Handle) in order to free memory*/
/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a buffer) */
MEDIAINFO_EXP size_t            __stdcall MediaInfoA_Open_Buffer (void* Handle, const unsigned char* Begin, size_t Begin_Size, const unsigned char* End, size_t End_Size); /*return Handle*/
/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a buffer, Init) */
MEDIAINFO_EXP size_t            __stdcall MediaInfoA_Open_Buffer_Init (void* Handle, MediaInfo_int64u File_Size, MediaInfo_int64u File_Offset);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a buffer, Continue) */
MEDIAINFO_EXP size_t            __stdcall MediaInfoA_Open_Buffer_Continue (void* Handle, MediaInfo_int8u* Buffer, size_t Buffer_Size);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a buffer, Continue_GoTo_Get) */
MEDIAINFO_EXP MediaInfo_int64u  __stdcall MediaInfoA_Open_Buffer_Continue_GoTo_Get (void* Handle);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a buffer, Finalize) */
MEDIAINFO_EXP size_t            __stdcall MediaInfoA_Open_Buffer_Finalize (void* Handle);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Save */
MEDIAINFO_EXP size_t            __stdcall MediaInfoA_Save (void* Handle);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Close */
MEDIAINFO_EXP void              __stdcall MediaInfoA_Close (void* Handle);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Inform */
MEDIAINFO_EXP const char*       __stdcall MediaInfoA_Inform (void* Handle, size_t Reserved); /*Default : Reserved=MediaInfo_*/
/** @brief Wrapper for MediaInfoLib::MediaInfo::Get */
MEDIAINFO_EXP const char*       __stdcall MediaInfoA_GetI (void* Handle, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, MediaInfo_info_C InfoKind); /*Default : InfoKind=Info_Text*/
/** @brief Wrapper for MediaInfoLib::MediaInfo::Get */
MEDIAINFO_EXP const char*       __stdcall MediaInfoA_Get (void* Handle, MediaInfo_stream_C StreamKind, size_t StreamNumber, const char* Parameter, MediaInfo_info_C InfoKind, MediaInfo_info_C SearchKind); /*Default : InfoKind=Info_Text, SearchKind=Info_Name*/
/** @brief Wrapper for MediaInfoLib::MediaInfo::Set */
MEDIAINFO_EXP size_t            __stdcall MediaInfoA_SetI (void* Handle, const char* ToSet, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, const char* OldParameter);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Set */
MEDIAINFO_EXP size_t            __stdcall MediaInfoA_Set (void* Handle, const char* ToSet, MediaInfo_stream_C StreamKind, size_t StreamNumber, const char* Parameter, const char* OldParameter);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Option */
MEDIAINFO_EXP const char*       __stdcall MediaInfoA_Option (void* Handle, const char* Option, const char* Value);
/** @brief Wrapper for MediaInfoLib::MediaInfo::State_Get */
MEDIAINFO_EXP size_t            __stdcall MediaInfoA_State_Get (void* Handle);
/** @brief Wrapper for MediaInfoLib::MediaInfo::Count_Get */
MEDIAINFO_EXP size_t            __stdcall MediaInfoA_Count_Get (void* Handle, MediaInfo_stream_C StreamKind, size_t StreamNumber); /*Default : StreamNumber=-1*/


#if defined (MEDIAINFO_DLL_EXPORT) || (defined (UNICODE) || defined (_UNICODE)) //DLL construction or Unicode
/** @brief A 'new' MediaInfoList interface, return a Handle, don't forget to delete it after using it*/
MEDIAINFO_EXP void*             __stdcall MediaInfoList_New (); /*you must ALWAYS call MediaInfoList_Delete(Handle) in order to free memory*/
/** @brief A 'new' MediaInfoList interface (with a quick init of useful options : "**VERSION**;**APP_NAME**;**APP_VERSION**", but without debug information, use it only if you know what you do), return a Handle, don't forget to delete it after using it*/
MEDIAINFO_EXP void*             __stdcall MediaInfoList_New_Quick (const wchar_t* Files, const wchar_t* Config); /*you must ALWAYS call MediaInfoList_Delete(Handle) in order to free memory*/
/** @brief Delete a MediaInfoList interface*/
MEDIAINFO_EXP void              __stdcall MediaInfoList_Delete (void* Handle);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Open (with a filename)*/
MEDIAINFO_EXP size_t            __stdcall MediaInfoList_Open (void* Handle, const wchar_t* Files, const MediaInfo_fileoptions_C Options); /*Default : Options=MediaInfo_FileOption_Nothing*/
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Open (with a buffer) */
MEDIAINFO_EXP size_t            __stdcall MediaInfoList_Open_Buffer (void* Handle, const unsigned char* Begin, size_t Begin_Size, const unsigned char* End, size_t End_Size); /*return Handle*/
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Save */
MEDIAINFO_EXP size_t            __stdcall MediaInfoList_Save (void* Handle, size_t FilePos);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Close */
MEDIAINFO_EXP void              __stdcall MediaInfoList_Close (void* Handle, size_t FilePos);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Inform */
MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfoList_Inform (void* Handle, size_t FilePos, size_t Reserved); /*Default : Reserved=0*/
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Get */
MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfoList_GetI (void* Handle, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, MediaInfo_info_C InfoKind); /*Default : InfoKind=Info_Text*/
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Get */
MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfoList_Get (void* Handle, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber, const wchar_t* Parameter, MediaInfo_info_C InfoKind, MediaInfo_info_C SearchKind); /*Default : InfoKind=Info_Text, SearchKind=Info_Name*/
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Set */
MEDIAINFO_EXP size_t            __stdcall MediaInfoList_SetI (void* Handle, const wchar_t* ToSet, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, const wchar_t* OldParameter);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Set */
MEDIAINFO_EXP size_t            __stdcall MediaInfoList_Set (void* Handle, const wchar_t* ToSet, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber, const wchar_t* Parameter, const wchar_t* OldParameter);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Option */
MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfoList_Option (void* Handle, const wchar_t* Option, const wchar_t* Value);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::State_Get */
MEDIAINFO_EXP size_t            __stdcall MediaInfoList_State_Get (void* Handle);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Count_Get */
MEDIAINFO_EXP size_t            __stdcall MediaInfoList_Count_Get (void* Handle, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber); /*Default : StreamNumber=-1*/
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Count_Get */
MEDIAINFO_EXP size_t            __stdcall MediaInfoList_Count_Get_Files (void* Handle);
#else //defined (MEDIAINFO_DLL_EXPORT) || (defined (UNICODE) || defined (_UNICODE))
    #define MediaInfoList_New               MediaInfoListA_New
    #define MediaInfoList_New_Quick         MediaInfoListA_New_Quick
    #define MediaInfoList_Delete            MediaInfoListA_Delete
    #define MediaInfoList_Open              MediaInfoListA_Open
    #define MediaInfoList_Open_Buffer       MediaInfoListA_Open_Buffer
    #define MediaInfoList_Save              MediaInfoListA_Save
    #define MediaInfoList_Save_All          MediaInfoListA_Save_All
    #define MediaInfoList_Close             MediaInfoListA_Close
    #define MediaInfoList_Close_All         MediaInfoListA_Close_All
    #define MediaInfoList_Inform            MediaInfoListA_Inform
    #define MediaInfoList_Inform_All        MediaInfoListA_Inform_All
    #define MediaInfoList_GetI              MediaInfoListA_GetI
    #define MediaInfoList_Get               MediaInfoListA_Get
    #define MediaInfoList_SetI              MediaInfoListA_SetI
    #define MediaInfoList_Set               MediaInfoListA_Set
    #define MediaInfoList_Option            MediaInfoListA_Option
    #define MediaInfoList_State_Get         MediaInfoListA_State_Get
    #define MediaInfoList_Count_Get         MediaInfoListA_Count_Get
    #define MediaInfoList_Count_Get_Files   MediaInfoListA_Count_Get_Files
#endif //defined (MEDIAINFO_DLL_EXPORT) || (defined (UNICODE) || defined (_UNICODE))

/* Warning : Deprecated, use MediaInfo_Option("Info_Version", "**YOUR VERSION COMPATIBLE**") instead */
MEDIAINFO_EXP const char*       __stdcall MediaInfo_Info_Version ();


/** @brief A 'new' MediaInfoList interface, return a Handle, don't forget to delete it after using it*/
MEDIAINFO_EXP void*             __stdcall MediaInfoListA_New (); /*you must ALWAYS call MediaInfoList_Delete(Handle) in order to free memory*/
/** @brief A 'new' MediaInfoList interface (with a quick init of useful options : "**VERSION**;**APP_NAME**;**APP_VERSION**", but without debug information, use it only if you know what you do), return a Handle, don't forget to delete it after using it*/
MEDIAINFO_EXP void*             __stdcall MediaInfoListA_New_Quick (const char* Files, const char* Config); /*you must ALWAYS call MediaInfoList_Delete(Handle) in order to free memory*/
/** @brief Delete a MediaInfoList interface*/
MEDIAINFO_EXP void              __stdcall MediaInfoListA_Delete (void* Handle);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Open (with a filename)*/
MEDIAINFO_EXP size_t            __stdcall MediaInfoListA_Open (void* Handle, const char* Files, const MediaInfo_fileoptions_C Options); /*Default : Options=0*/
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Open (with a buffer) */
MEDIAINFO_EXP size_t            __stdcall MediaInfoListA_Open_Buffer (void* Handle, const unsigned char* Begin, size_t Begin_Size, const unsigned char* End, size_t End_Size); /*return Handle*/
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Save */
MEDIAINFO_EXP size_t            __stdcall MediaInfoListA_Save (void* Handle, size_t FilePos);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Close */
MEDIAINFO_EXP void              __stdcall MediaInfoListA_Close (void* Handle, size_t FilePos);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Inform */
MEDIAINFO_EXP const char*       __stdcall MediaInfoListA_Inform (void* Handle, size_t FilePos, size_t Reserved); /*Default : Reserved=0*/
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Get */
MEDIAINFO_EXP const char*       __stdcall MediaInfoListA_GetI (void* Handle, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, MediaInfo_info_C InfoKind); /*Default : InfoKind=Info_Text*/
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Get */
MEDIAINFO_EXP const char*       __stdcall MediaInfoListA_Get (void* Handle, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber, const char* Parameter, MediaInfo_info_C InfoKind, MediaInfo_info_C SearchKind); /*Default : InfoKind=Info_Text, SearchKind=Info_Name*/
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Set */
MEDIAINFO_EXP size_t            __stdcall MediaInfoListA_SetI (void* Handle, const char* ToSet, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, const char* OldParameter);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Set */
MEDIAINFO_EXP size_t            __stdcall MediaInfoListA_Set (void* Handles, const char* ToSet, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber, const char* Parameter, const char* OldParameter);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Option */
MEDIAINFO_EXP const char*       __stdcall MediaInfoListA_Option (void* Handle, const char* Option, const char* Value);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::State_Get */
MEDIAINFO_EXP size_t            __stdcall MediaInfoListA_State_Get (void* Handle);
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Count_Get */
MEDIAINFO_EXP size_t            __stdcall MediaInfoListA_Count_Get (void* Handle, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber); /*Default : StreamNumber=-1*/
/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Count_Get */
MEDIAINFO_EXP size_t            __stdcall MediaInfoListA_Count_Get_Files (void* Handle);

#ifdef __cplusplus
}
#endif /*__cplusplus*/



#ifdef __cplusplus
//DLL C++ wrapper for C functions
#if !defined(MediaInfoH) && !defined (MEDIAINFO_DLL_EXPORT) //No Lib include and No DLL construction

//---------------------------------------------------------------------------
#include <string>
//---------------------------------------------------------------------------

namespace MediaInfoDLL
{

//---------------------------------------------------------------------------
//Char types
#undef  _T
#define _T(__x)     __T(__x)
#if defined(UNICODE) || defined (_UNICODE)
    typedef wchar_t Char;
    #undef  __T
    #define __T(__x) L ## __x
#else
    typedef char Char;
    #undef  __T
    #define __T(__x) __x
#endif
typedef std::basic_string<Char>        String;
typedef std::basic_stringstream<Char>  StringStream;
typedef std::basic_istringstream<Char> tiStringStream;
typedef std::basic_ostringstream<Char> toStringStream;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
/// @brief Kinds of Stream
enum stream_t
{
    Stream_General,                 ///< StreamKind = General
    Stream_Video,                   ///< StreamKind = Video
    Stream_Audio,                   ///< StreamKind = Audio
    Stream_Text,                    ///< StreamKind = Text
    Stream_Chapters,                ///< StreamKind = Chapters
    Stream_Image,                   ///< StreamKind = Image
    Stream_Menu,                    ///< StreamKind = Menu
    Stream_Max,
};

/// @brief Kind of information
enum info_t
{
    Info_Name,                      ///< InfoKind = Unique name of parameter
    Info_Text,                      ///< InfoKind = Value of parameter
    Info_Measure,                   ///< InfoKind = Unique name of measure unit of parameter
    Info_Options,                   ///< InfoKind = See infooptions_t
    Info_Name_Text,                 ///< InfoKind = Translated name of parameter
    Info_Measure_Text,              ///< InfoKind = Translated name of measure unit
    Info_Info,                      ///< InfoKind = More information about the parameter
    Info_HowTo,                     ///< InfoKind = Information : how data is found
    Info_Max
};

/// Get(...)[infooptions_t] return a string like "YNYN..." \n
/// Use this enum to know at what correspond the Y (Yes) or N (No)
/// If Get(...)[0]==Y, then :
/// @brief Option if InfoKind = Info_Options
enum infooptions_t
{
    InfoOption_ShowInInform,        ///< Show this parameter in Inform()
    InfoOption_Reserved,            ///<
    InfoOption_ShowInSupported,        ///< Internal use only (info : Must be showed in Info_Capacities() )
    InfoOption_TypeOfValue,         ///< Value return by a standard Get() can be : T (Text), I (Integer, warning up to 64 bits), F (Float), D (Date), B (Binary datas coded Base64) (Numbers are in Base 10)
    InfoOption_Max
};

/// @brief File opening options
enum fileoptions_t
{
    FileOption_Nothing      =0x00,
    FileOption_NoRecursive  =0x01,  ///< Do not browse folders recursively
    FileOption_CloseAll     =0x02,  ///< Close all files before open
    FileOption_Max          =0x04
};

//---------------------------------------------------------------------------
class MediaInfo
{
public :
    MediaInfo ()                {Handle=MediaInfo_New();};
    ~MediaInfo ()               {MediaInfo_Delete(Handle);};

    //File
    size_t Open (const String &File) {return MediaInfo_Open(Handle, File.c_str());};
    size_t Open (const unsigned char* Begin, size_t Begin_Size, const unsigned char* End=NULL, size_t End_Size=NULL) {return MediaInfo_Open_Buffer(Handle, Begin, Begin_Size, End, End_Size);};
    size_t Save () {return MediaInfo_Save(Handle);};
    void Close () {return MediaInfo_Close(Handle);};

    //General information
    String Inform ()  {return MediaInfo_Inform(Handle, 0);};
    String Get (stream_t StreamKind, size_t StreamNumber, size_t Parameter, info_t InfoKind=Info_Text)  {return MediaInfo_GetI (Handle, (MediaInfo_stream_C)StreamKind, StreamNumber, Parameter, (MediaInfo_info_C)InfoKind);};
    String Get (stream_t StreamKind, size_t StreamNumber, const String &Parameter, info_t InfoKind=Info_Text, info_t SearchKind=Info_Name)  {return MediaInfo_Get (Handle, (MediaInfo_stream_C)StreamKind, StreamNumber, Parameter.c_str(), (MediaInfo_info_C)InfoKind, (MediaInfo_info_C)SearchKind);};
    size_t Set (const String &ToSet, stream_t StreamKind, size_t StreamNumber, size_t Parameter, const String &OldValue=_T(""))  {return MediaInfo_SetI (Handle, ToSet.c_str(), (MediaInfo_stream_C)StreamKind, StreamNumber, Parameter, OldValue.c_str());};
    size_t Set (const String &ToSet, stream_t StreamKind, size_t StreamNumber, const String &Parameter, const String &OldValue=_T(""))  {return MediaInfo_Set (Handle, ToSet.c_str(), (MediaInfo_stream_C)StreamKind, StreamNumber, Parameter.c_str(), OldValue.c_str());};
    String        Option (const String &Option, const String &Value=_T(""))  {return MediaInfo_Option (Handle, Option.c_str(), Value.c_str());};
    static String Option_Static (const String &Option, const String &Value=_T(""))  {return MediaInfo_Option (NULL, Option.c_str(), Value.c_str());};
    size_t                  State_Get ()  {return MediaInfo_State_Get(Handle);};
    size_t                  Count_Get (stream_t StreamKind, size_t StreamNumber=-1)  {return MediaInfo_Count_Get(Handle, (MediaInfo_stream_C)StreamKind, StreamNumber);};

private :
    void* Handle;
};

class MediaInfoList
{
public :
    MediaInfoList ()                {Handle=MediaInfoList_New();};
    ~MediaInfoList ()               {MediaInfoList_Delete(Handle);};

    //File
    size_t Open (const String &File, const fileoptions_t Options=FileOption_Nothing) {return MediaInfoList_Open(Handle, File.c_str(), (MediaInfo_fileoptions_C)Options);};
    size_t Open (const unsigned char* Begin, size_t Begin_Size, const unsigned char* End=NULL, size_t End_Size=NULL) {return MediaInfoList_Open_Buffer(Handle, Begin, Begin_Size, End, End_Size);};
    size_t Save (size_t FilePos) {return MediaInfoList_Save(Handle, FilePos);};
    void Close (size_t FilePos=-1) {return MediaInfoList_Close(Handle, FilePos);};

    //General information
    String Inform (size_t FilePos=-1)  {return MediaInfoList_Inform(Handle, FilePos, 0);};
    String Get (size_t FilePos, stream_t StreamKind, size_t StreamNumber, size_t Parameter, info_t InfoKind=Info_Text)  {return MediaInfoList_GetI (Handle, FilePos, (MediaInfo_stream_C)StreamKind, StreamNumber, Parameter, (MediaInfo_info_C)InfoKind);};
    String Get (size_t FilePos, stream_t StreamKind, size_t StreamNumber, const String &Parameter, info_t InfoKind=Info_Text, info_t SearchKind=Info_Name)  {return MediaInfoList_Get (Handle, FilePos, (MediaInfo_stream_C)StreamKind, StreamNumber, Parameter.c_str(), (MediaInfo_info_C)InfoKind, (MediaInfo_info_C)SearchKind);};
    size_t Set (const String &ToSet, size_t FilePos, stream_t StreamKind, size_t StreamNumber, size_t Parameter, const String &OldValue=_T(""))  {return MediaInfoList_SetI (Handle, ToSet.c_str(), FilePos, (MediaInfo_stream_C)StreamKind, StreamNumber, Parameter, OldValue.c_str());};
    size_t Set (const String &ToSet, size_t FilePos, stream_t StreamKind, size_t StreamNumber, const String &Parameter, const String &OldValue=_T(""))  {return MediaInfoList_Set (Handle, ToSet.c_str(), FilePos, (MediaInfo_stream_C)StreamKind, StreamNumber, Parameter.c_str(), OldValue.c_str());};
    String        Option (const String &Option, const String &Value=_T(""))  {return MediaInfoList_Option (Handle, Option.c_str(), Value.c_str());};
    static String Option_Static (const String &Option, const String &Value=_T(""))  {return MediaInfoList_Option (NULL, Option.c_str(), Value.c_str());};
    size_t                  State_Get ()  {return MediaInfoList_State_Get(Handle);};
    size_t                  Count_Get (size_t FilePos, stream_t StreamKind, size_t StreamNumber=-1)  {return MediaInfoList_Count_Get(Handle, FilePos, (MediaInfo_stream_C)StreamKind, StreamNumber);};
    size_t                  Count_Get ()  {return MediaInfoList_Count_Get_Files(Handle);};

private :
    void* Handle;
};

} //NameSpace
#endif//#if !defined(MediaInfoH) && !defined (MEDIAINFO_DLL_EXPORT) && !(defined (UNICODE) || defined (_UNICODE))
#endif /*__cplusplus*/


#endif
