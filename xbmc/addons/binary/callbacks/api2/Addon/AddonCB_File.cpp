/*
 *      Copyright (C) 2015 Team KODI
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

#include "AddonCB_Directory.h"

#include "FileItem.h"
#include "Util.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/callbacks/api2/AddonCallbacksBase.h"
#include "utils/Crc32.h"
#include "filesystem/File.h"

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

CAddOnFile::CAddOnFile()
{

}

void CAddOnFile::Init(V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->File.open_file            = CAddOnFile::open_file;
  callbacks->File.open_file_for_write  = CAddOnFile::open_file_for_write;
  callbacks->File.read_file            = CAddOnFile::read_file;
  callbacks->File.read_file_string     = CAddOnFile::read_file_string;
  callbacks->File.write_file           = CAddOnFile::write_file;
  callbacks->File.flush_file           = CAddOnFile::flush_file;
  callbacks->File.seek_file            = CAddOnFile::seek_file;
  callbacks->File.truncate_file        = CAddOnFile::truncate_file;
  callbacks->File.get_file_position    = CAddOnFile::get_file_position;
  callbacks->File.get_file_length      = CAddOnFile::get_file_length;
  callbacks->File.close_file           = CAddOnFile::close_file;
  callbacks->File.get_file_chunk_size  = CAddOnFile::get_file_chunk_size;
  callbacks->File.file_exists          = CAddOnFile::file_exists;
  callbacks->File.stat_file            = CAddOnFile::stat_file;
  callbacks->File.delete_file          = CAddOnFile::delete_file;
  callbacks->File.get_file_md5         = CAddOnFile::get_file_md5;
  callbacks->File.get_cache_thumb_name = CAddOnFile::get_cache_thumb_name;
  callbacks->File.make_legal_filename  = CAddOnFile::make_legal_filename;
  callbacks->File.make_legal_path      = CAddOnFile::make_legal_path;
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

}; /* extern "C" */
}; /* namespace AddOn */

}; /* namespace KodiAPI */
}; /* namespace V2 */
