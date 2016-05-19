#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>

#ifdef _WIN32                   // windows
#ifndef _SSIZE_T_DEFINED
typedef intptr_t      ssize_t;
#define _SSIZE_T_DEFINED
#endif // !_SSIZE_T_DEFINED
#endif

#if !defined(__stat64)
  #if defined(__APPLE__)
    #define __stat64 stat
  #else
    #define __stat64 stat64
  #endif
#endif

struct __stat64;

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;

namespace AddOn
{
extern "C"
{

  class CAddOnFile
  {
  public:
    static void Init(struct CB_AddOnLib *interfaces);

    static void* open_file(
          void*                     hdl,
          const char*               strFileName,
          unsigned int              flags);

    static void* open_file_for_write(
          void*                     hdl,
          const char*               strFileName,
          bool                      bOverwrite);

    static ssize_t read_file(
          void*                     hdl,
          void*                     file,
          void*                     lpBuf,
          size_t                    uiBufSize);

    static bool read_file_string(
          void*                     hdl,
          void*                     file,
          char*                     szLine,
          int                       iLineLength);

    static ssize_t write_file(
          void*                     hdl,
          void*                     file,
          const void*               lpBuf,
          size_t                    uiBufSize);

    static void flush_file(
          void*                     hdl,
          void*                     file);

    static int64_t seek_file(
          void*                     hdl,
          void*                     file,
          int64_t                   iFilePosition,
          int                       iWhence);

    static int truncate_file(
          void*                     hdl,
          void*                     file,
          int64_t                   iSize);

    static int64_t get_file_position(
          void*                     hdl,
          void*                     file);

    static int64_t get_file_length(
          void*                     hdl,
          void*                     file);

    static double get_file_download_speed(
          void*                     hdl,
          void*                     file);

    static void close_file(
          void*                     hdl,
          void*                     file);

    static int get_file_chunk_size(
          void*                     hdl,
          void*                     file);

    static bool file_exists(
          void*                     hdl,
          const char*               strFileName,
          bool                      bUseCache);

    static int stat_file(
          void*                     hdl,
          const char*               strFileName,
          struct __stat64*          buffer);

    static bool delete_file(
          void*                     hdl,
          const char*               strFileName);

    static char* get_file_md5(
          void*                     hdl,
          const char*               strFileName);

    static char* get_cache_thumb_name(
          void*                     hdl,
          const char*               strFileName);

    static char* make_legal_filename(
          void*                     hdl,
          const char*               strFileName);

    static char* make_legal_path(
          void*                     hdl,
          const char*               strPath);

    static void* curl_create(
          void*                     hdl,
          const char*               url);

    static bool curl_add_option(
          void*                     hdl,
          void*                     file,
          int                       type,
          const char*               name,
          const char*               value);

    static bool curl_open(
          void*                     hdl,
          void*                     file,
          unsigned int              flags);
  };

} /* extern "C" */
} /* namespace AddOn */

} /* namespace KodiAPI */
} /* namespace V2 */
