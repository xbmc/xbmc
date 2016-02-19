/*
 *      Copyright (C) 2016 Team KODI
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

#include "InterProcess_Addon_VFSUtils.h"
#include "InterProcess.h"

#include <string>
#include <cstring>

extern "C"
{

  bool CKODIAddon_InterProcess_Addon_VFSUtils::CreateDirectory(const std::string& strPath)
  {
    return g_interProcess.m_Callbacks->Directory.create_directory(g_interProcess.m_Handle, strPath.c_str());
  }

  bool CKODIAddon_InterProcess_Addon_VFSUtils::DirectoryExists(const std::string& strPath)
  {
    return g_interProcess.m_Callbacks->Directory.directory_exists(g_interProcess.m_Handle, strPath.c_str());
  }

  bool CKODIAddon_InterProcess_Addon_VFSUtils::RemoveDirectory(const std::string& strPath)
  {
    return g_interProcess.m_Callbacks->Directory.remove_directory(g_interProcess.m_Handle, strPath.c_str());
  }

  bool CKODIAddon_InterProcess_Addon_VFSUtils::GetVFSDirectory(const char *strPath, const char* mask, VFSDirEntry** items, unsigned int* num_items)
  {
    return g_interProcess.m_Callbacks->VFS.get_vfs_directory(g_interProcess.m_Handle, strPath, mask, items, num_items);
  }

  void CKODIAddon_InterProcess_Addon_VFSUtils::FreeVFSDirectory(VFSDirEntry* items, unsigned int num_items)
  {
    return g_interProcess.m_Callbacks->VFS.free_vfs_directory(g_interProcess.m_Handle, items, num_items);
  }

  std::string CKODIAddon_InterProcess_Addon_VFSUtils::GetFileMD5(const std::string& strFileName)
  {
    std::string strReturn;
    char* strMd5 = g_interProcess.m_Callbacks->File.get_file_md5(g_interProcess.m_Handle, strFileName.c_str());
    if (strMd5 != nullptr)
    {
      if (std::strlen(strMd5))
        strReturn = strMd5;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strMd5);
    }
    return strReturn;
  }

  std::string CKODIAddon_InterProcess_Addon_VFSUtils::GetCacheThumbName(const std::string& strFileName)
  {
    std::string strReturn;
    char* strThumbName = g_interProcess.m_Callbacks->File.get_cache_thumb_name(g_interProcess.m_Handle, strFileName.c_str());
    if (strThumbName != nullptr)
    {
      if (std::strlen(strThumbName))
        strReturn = strThumbName;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strThumbName);
    }
    return strReturn;
  }

  std::string CKODIAddon_InterProcess_Addon_VFSUtils::MakeLegalFileName(const std::string& strFileName)
  {
    std::string strReturn;
    char* strLegalFileName = g_interProcess.m_Callbacks->File.make_legal_filename(g_interProcess.m_Handle, strFileName.c_str());
    if (strLegalFileName != nullptr)
    {
      if (std::strlen(strLegalFileName))
        strReturn = strLegalFileName;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strLegalFileName);
    }
    return strReturn;
  }

  std::string CKODIAddon_InterProcess_Addon_VFSUtils::MakeLegalPath(const std::string& strPath)
  {
    std::string strReturn;
    char* strLegalPath = g_interProcess.m_Callbacks->File.make_legal_path(g_interProcess.m_Handle, strPath.c_str());
    if (strLegalPath != nullptr)
    {
      if (std::strlen(strLegalPath))
        strReturn = strLegalPath;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strLegalPath);
    }
    return strReturn;
  }

  void* CKODIAddon_InterProcess_Addon_VFSUtils::OpenFile(const std::string& strFileName, unsigned int flags)
  {
    return g_interProcess.m_Callbacks->File.open_file(g_interProcess.m_Handle, strFileName.c_str(), flags);
  }

  void* CKODIAddon_InterProcess_Addon_VFSUtils::OpenFileForWrite(const std::string& strFileName, bool bOverWrite)
  {
    return g_interProcess.m_Callbacks->File.open_file_for_write(g_interProcess.m_Handle, strFileName.c_str(), bOverWrite);
  }

  ssize_t CKODIAddon_InterProcess_Addon_VFSUtils::ReadFile(void* file, void* lpBuf, size_t uiBufSize)
  {
    return g_interProcess.m_Callbacks->File.read_file(g_interProcess.m_Handle, file, lpBuf, uiBufSize);
  }

  bool CKODIAddon_InterProcess_Addon_VFSUtils::ReadFileString(void* file, char *szLine, int iLineLength)
  {
    return g_interProcess.m_Callbacks->File.read_file_string(g_interProcess.m_Handle, file, szLine, iLineLength);
  }

  ssize_t CKODIAddon_InterProcess_Addon_VFSUtils::WriteFile(void* file, const void* lpBuf, size_t uiBufSize)
  {
    return g_interProcess.m_Callbacks->File.write_file(g_interProcess.m_Handle, file, lpBuf, uiBufSize);
  }

  void CKODIAddon_InterProcess_Addon_VFSUtils::FlushFile(void* file)
  {
     return g_interProcess.m_Callbacks->File.flush_file(g_interProcess.m_Handle, file);
  }

  int64_t CKODIAddon_InterProcess_Addon_VFSUtils::SeekFile(void* file, int64_t iFilePosition, int iWhence)
  {
    return g_interProcess.m_Callbacks->File.seek_file(g_interProcess.m_Handle, file, iFilePosition, iWhence);
  }

  int CKODIAddon_InterProcess_Addon_VFSUtils::TruncateFile(void* file, int64_t iSize)
  {
    return g_interProcess.m_Callbacks->File.truncate_file(g_interProcess.m_Handle, file, iSize);
  }

  int64_t CKODIAddon_InterProcess_Addon_VFSUtils::GetFilePosition(void* file)
  {
    return g_interProcess.m_Callbacks->File.get_file_position(g_interProcess.m_Handle, file);
  }

  int64_t CKODIAddon_InterProcess_Addon_VFSUtils::GetFileLength(void* file)
  {
    return g_interProcess.m_Callbacks->File.get_file_length(g_interProcess.m_Handle, file);
  }

  void CKODIAddon_InterProcess_Addon_VFSUtils::CloseFile(void* file)
  {
    return g_interProcess.m_Callbacks->File.close_file(g_interProcess.m_Handle, file);
  }

  int CKODIAddon_InterProcess_Addon_VFSUtils::GetFileChunkSize(void* file)
  {
    return g_interProcess.m_Callbacks->File.get_file_chunk_size(g_interProcess.m_Handle, file);
  }

  bool CKODIAddon_InterProcess_Addon_VFSUtils::FileExists(const std::string& strFileName, bool bUseCache)
  {
    return g_interProcess.m_Callbacks->File.file_exists(g_interProcess.m_Handle, strFileName.c_str(), bUseCache);
  }

  int CKODIAddon_InterProcess_Addon_VFSUtils::StatFile(const std::string& strFileName, struct __stat64* buffer)
  {
    return g_interProcess.m_Callbacks->File.stat_file(g_interProcess.m_Handle, strFileName.c_str(), buffer);
  }

  bool CKODIAddon_InterProcess_Addon_VFSUtils::DeleteFile(const std::string& strFileName)
  {
    return g_interProcess.m_Callbacks->File.delete_file(g_interProcess.m_Handle, strFileName.c_str());
  }



}; /* extern "C" */
