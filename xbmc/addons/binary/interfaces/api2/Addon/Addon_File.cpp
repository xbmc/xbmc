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

#include "FileItem.h"
#include "Util.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "utils/Crc32.h"
#include "filesystem/File.h"

#include "Addon_File.h"

using namespace ADDON;
using namespace XFILE;

namespace V2
{
namespace KodiAPI
{

namespace AddOn
{
extern "C"
{

void CAddOnFile::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->File.open_file               = V2::KodiAPI::AddOn::CAddOnFile::open_file;
  interfaces->File.open_file_for_write     = V2::KodiAPI::AddOn::CAddOnFile::open_file_for_write;
  interfaces->File.read_file               = V2::KodiAPI::AddOn::CAddOnFile::read_file;
  interfaces->File.read_file_string        = V2::KodiAPI::AddOn::CAddOnFile::read_file_string;
  interfaces->File.write_file              = V2::KodiAPI::AddOn::CAddOnFile::write_file;
  interfaces->File.flush_file              = V2::KodiAPI::AddOn::CAddOnFile::flush_file;
  interfaces->File.seek_file               = V2::KodiAPI::AddOn::CAddOnFile::seek_file;
  interfaces->File.truncate_file           = V2::KodiAPI::AddOn::CAddOnFile::truncate_file;
  interfaces->File.get_file_position       = V2::KodiAPI::AddOn::CAddOnFile::get_file_position;
  interfaces->File.get_file_length         = V2::KodiAPI::AddOn::CAddOnFile::get_file_length;
  interfaces->File.get_file_download_speed = V2::KodiAPI::AddOn::CAddOnFile::get_file_download_speed;
  interfaces->File.close_file              = V2::KodiAPI::AddOn::CAddOnFile::close_file;
  interfaces->File.get_file_chunk_size     = V2::KodiAPI::AddOn::CAddOnFile::get_file_chunk_size;
  interfaces->File.file_exists             = V2::KodiAPI::AddOn::CAddOnFile::file_exists;
  interfaces->File.stat_file               = V2::KodiAPI::AddOn::CAddOnFile::stat_file;
  interfaces->File.delete_file             = V2::KodiAPI::AddOn::CAddOnFile::delete_file;
  interfaces->File.get_file_md5            = V2::KodiAPI::AddOn::CAddOnFile::get_file_md5;
  interfaces->File.get_cache_thumb_name    = V2::KodiAPI::AddOn::CAddOnFile::get_cache_thumb_name;
  interfaces->File.make_legal_filename     = V2::KodiAPI::AddOn::CAddOnFile::make_legal_filename;
  interfaces->File.make_legal_path         = V2::KodiAPI::AddOn::CAddOnFile::make_legal_path;
  interfaces->File.curl_create             = V2::KodiAPI::AddOn::CAddOnFile::curl_create;
  interfaces->File.curl_add_option         = V2::KodiAPI::AddOn::CAddOnFile::curl_add_option;
  interfaces->File.curl_open               = V2::KodiAPI::AddOn::CAddOnFile::curl_open;
}

void* CAddOnFile::open_file(
        void*                     hdl,
        const char*               strFileName,
        unsigned int              flags)
{
  try
  {
    if (!hdl || !strFileName)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', strFileName='%p')",
                                        __FUNCTION__, hdl, strFileName);

    CFile* file = new CFile;
    if (file->Open(strFileName, flags))
      return ((void*)file);

    delete file;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void* CAddOnFile::open_file_for_write(
        void*                     hdl,
        const char*               strFileName,
        bool                      bOverwrite)
{
  try
  {
    if (!hdl || !strFileName)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', strFileName='%p')",
                                        __FUNCTION__, hdl, strFileName);

    CFile* file = new CFile;
    if (file->OpenForWrite(strFileName, bOverwrite))
      return ((void*)file);

    delete file;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

ssize_t CAddOnFile::read_file(
        void*                     hdl,
        void*                     file,
        void*                     lpBuf,
        size_t                    uiBufSize)
{
  try
  {
    if (!hdl || !file)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p')",
                                        __FUNCTION__, hdl, file);

    return static_cast<CFile*>(file)->Read(lpBuf, uiBufSize);
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

bool CAddOnFile::read_file_string(
        void*                     hdl,
        void*                     file,
        char*                     szLine,
        int                       iLineLength)
{
  try
  {
    if (!hdl || !file)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p')",
                                        __FUNCTION__, hdl, file);

    return static_cast<CFile*>(file)->ReadString(szLine, iLineLength);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

ssize_t CAddOnFile::write_file(
        void*                     hdl,
        void*                     file,
        const void*               lpBuf,
        size_t                    uiBufSize)
{
  try
  {
    if (!hdl || !file || !lpBuf)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p', lpBuf='%p')",
                                        __FUNCTION__, hdl, file, lpBuf);

    return static_cast<CFile*>(file)->Write(lpBuf, uiBufSize);
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

void CAddOnFile::flush_file(
        void*                     hdl,
        void*                     file)
{
  try
  {
    if (!hdl || !file)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p')",
                                        __FUNCTION__, hdl, file);

    static_cast<CFile*>(file)->Flush();
  }
  HANDLE_ADDON_EXCEPTION
}

int64_t CAddOnFile::seek_file(
        void*                     hdl,
        void*                     file,
        int64_t                   iFilePosition,
        int                       iWhence)
{
  try
  {
    if (!hdl || !file)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p')",
                                        __FUNCTION__, hdl, file);

    return static_cast<CFile*>(file)->Seek(iFilePosition, iWhence);
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

int CAddOnFile::truncate_file(
        void*                     hdl,
        void*                     file,
        int64_t                   iSize)
{
  try
  {
    if (!hdl || !file)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p')",
                                        __FUNCTION__, hdl, file);

    return static_cast<CFile*>(file)->Truncate(iSize);
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

int64_t CAddOnFile::get_file_position(
        void*                     hdl,
        void*                     file)
{
  try
  {
    if (!hdl || !file)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p')",
                                        __FUNCTION__, hdl, file);

    return static_cast<CFile*>(file)->GetPosition();
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

int64_t CAddOnFile::get_file_length(
        void*                     hdl,
        void*                     file)
{
  try
  {
    if (!hdl || !file)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p')",
                                        __FUNCTION__, hdl, file);

    return static_cast<CFile*>(file)->GetLength();
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

double CAddOnFile::get_file_download_speed(
        void*                     hdl,
        void*                     file)
{
  try
  {
    if (!hdl || !file)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p')",
                                        __FUNCTION__, hdl, file);

    return static_cast<CFile*>(file)->GetDownloadSpeed();
  }
  HANDLE_ADDON_EXCEPTION

  return 0.0f;
}

void CAddOnFile::close_file(
        void*                     hdl,
        void*                     file)
{
  try
  {
    if (!hdl || !file)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p')",
                                        __FUNCTION__, hdl, file);

    static_cast<CFile*>(file)->Close();
    delete static_cast<CFile*>(file);
  }
  HANDLE_ADDON_EXCEPTION
}

int CAddOnFile::get_file_chunk_size(
        void*                     hdl,
        void*                     file)
{
  try
  {
    if (!hdl || !file)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p')",
                                        __FUNCTION__, hdl, file);

    return static_cast<CFile*>(file)->GetChunkSize();
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

bool CAddOnFile::file_exists(
        void*                     hdl,
        const char*               strFileName,
        bool                      bUseCache)
{
  try
  {
    if (!hdl || !strFileName)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', strFileName='%p')",
                                        __FUNCTION__, hdl, strFileName);

    return CFile::Exists(strFileName, bUseCache);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

int CAddOnFile::stat_file(
        void*                     hdl,
        const char*               strFileName,
        struct ::__stat64*        buffer)
{
  try
  {
    if (!hdl || !strFileName)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', strFileName='%p')",
                                        __FUNCTION__, hdl, strFileName);

    return CFile::Stat(strFileName, buffer);
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

bool CAddOnFile::delete_file(
        void*                     hdl,
        const char*               strFileName)
{
  try
  {
    if (!hdl || !strFileName)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', strFileName='%p')",
                                        __FUNCTION__, hdl, strFileName);

    return CFile::Delete(strFileName);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

char* CAddOnFile::get_file_md5(void* hdl, const char* strFileName)
{
  try
  {
    if (!hdl || !strFileName)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', strFileName='%p')",
                                        __FUNCTION__, hdl, strFileName);

    std::string string = CUtil::GetFileMD5(strFileName);
    char* buffer = strdup(string.c_str());
    return buffer;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

char* CAddOnFile::get_cache_thumb_name(void* hdl, const char* strFileName)
{
  try
  {
    if (!hdl || !strFileName)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', strFileName='%p')",
                                        __FUNCTION__, hdl, strFileName);
    Crc32 crc;
    crc.ComputeFromLowerCase(strFileName);
    std::string string = StringUtils::Format("%08x.tbn", (unsigned __int32)crc);
    char* buffer = strdup(string.c_str());
    return buffer;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

char* CAddOnFile::make_legal_filename(void* hdl, const char* strFileName)
{
  try
  {
    if (!hdl || !strFileName)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', strFileName='%p')",
                                        __FUNCTION__, hdl, strFileName);

    std::string string = CUtil::MakeLegalFileName(strFileName);;
    char* buffer = strdup(string.c_str());
    return buffer;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

char* CAddOnFile::make_legal_path(void* hdl, const char* strPath)
{
  try
  {
    if (!hdl || !strPath)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', strPath='%p')",
                                        __FUNCTION__, hdl, strPath);

    std::string string = CUtil::MakeLegalPath(strPath);;
    char* buffer = strdup(string.c_str());
    return buffer;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void* CAddOnFile::curl_create(
          void*                     hdl,
          const char*               url)
{
  try
  {
    if (!hdl || !url)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', url='%p')", __FUNCTION__, hdl, url);

    CFile* file = new CFile;
    if (file->CURLCreate(url))
      return static_cast<void*>(file);

    delete file;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

bool CAddOnFile::curl_add_option(
          void*                     hdl,
          void*                     file,
          int                       type,
          const char*               name,
          const char*               value)
{
  try
  {
    if (!hdl || !file || !name || !value)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p', name='%p', value='%p')", __FUNCTION__, hdl, file, name, value);

    XFILE::CURLOPTIONTYPE internalType;
    switch (type)
    {
    case ADDON_CURL_OPTION_OPTION:
      internalType = XFILE::CURL_OPTION_OPTION;
      break;
    case ADDON_CURL_OPTION_PROTOCOL:
      internalType = XFILE::CURL_OPTION_PROTOCOL;
      break;
    case ADDON_CURL_OPTION_CREDENTIALS:
      internalType = XFILE::CURL_OPTION_CREDENTIALS;
      break;
    case ADDON_CURL_OPTION_HEADER:
      internalType = XFILE::CURL_OPTION_HEADER;
      break;
    default:
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid curl option type");
    };

    CFile* cfile = static_cast<CFile*>(file);
    if (cfile)
      return cfile->CURLAddOption(internalType, name, value);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnFile::curl_open(
          void*                     hdl,
          void*                     file,
          unsigned int              flags)
{
  try
  {
    if (!hdl || !file)
      throw ADDON::WrongValueException("CAddOnFile - %s - invalid data (handle='%p', file='%p')", __FUNCTION__, hdl, file);

    CFile* cfile = static_cast<CFile*>(file);
    if (cfile)
      return cfile->CURLOpen(flags);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

} /* extern "C" */
} /* namespace AddOn */

} /* namespace KodiAPI */
} /* namespace V2 */
