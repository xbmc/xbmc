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
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <cstring>
#include <iostream>       // std::cerr
#include <stdexcept>      // std::out_of_range

using namespace P8PLATFORM;

extern "C"
{

  bool CKODIAddon_InterProcess_Addon_VFSUtils::CreateDirectory(const std::string& strPath)
  {

  }

  bool CKODIAddon_InterProcess_Addon_VFSUtils::DirectoryExists(const std::string& strPath)
  {

  }

  bool CKODIAddon_InterProcess_Addon_VFSUtils::RemoveDirectory(const std::string& strPath)
  {

  }

  bool CKODIAddon_InterProcess_Addon_VFSUtils::GetVFSDirectory(const char *strPath, const char* mask, VFSDirEntry** items, unsigned int* num_items)
  {

  }

  void CKODIAddon_InterProcess_Addon_VFSUtils::FreeVFSDirectory(VFSDirEntry* items, unsigned int num_items)
  {

  }

  std::string CKODIAddon_InterProcess_Addon_VFSUtils::GetFileMD5(const std::string& strFileName)
  {

  }

  std::string CKODIAddon_InterProcess_Addon_VFSUtils::GetCacheThumbName(const std::string& strFileName)
  {

  }

  std::string CKODIAddon_InterProcess_Addon_VFSUtils::MakeLegalFileName(const std::string& strFileName)
  {

  }

  std::string CKODIAddon_InterProcess_Addon_VFSUtils::MakeLegalPath(const std::string& strPath)
  {

  }

  void* CKODIAddon_InterProcess_Addon_VFSUtils::OpenFile(const std::string& strFileName, unsigned int flags)
  {

  }

  void* CKODIAddon_InterProcess_Addon_VFSUtils::OpenFileForWrite(const std::string& strFileName, bool bOverWrite)
  {

  }

  ssize_t CKODIAddon_InterProcess_Addon_VFSUtils::ReadFile(void* file, void* lpBuf, size_t uiBufSize)
  {

  }

  bool CKODIAddon_InterProcess_Addon_VFSUtils::ReadFileString(void* file, char *szLine, int iLineLength)
  {

  }

  ssize_t CKODIAddon_InterProcess_Addon_VFSUtils::WriteFile(void* file, const void* lpBuf, size_t uiBufSize)
  {

  }

  void CKODIAddon_InterProcess_Addon_VFSUtils::FlushFile(void* file)
  {

  }

  int64_t CKODIAddon_InterProcess_Addon_VFSUtils::SeekFile(void* file, int64_t iFilePosition, int iWhence)
  {

  }

  int CKODIAddon_InterProcess_Addon_VFSUtils::TruncateFile(void* file, int64_t iSize)
  {

  }

  int64_t CKODIAddon_InterProcess_Addon_VFSUtils::GetFilePosition(void* file)
  {

  }

  int64_t CKODIAddon_InterProcess_Addon_VFSUtils::GetFileLength(void* file)
  {

  }

  void CKODIAddon_InterProcess_Addon_VFSUtils::CloseFile(void* file)
  {

  }

  int CKODIAddon_InterProcess_Addon_VFSUtils::GetFileChunkSize(void* file)
  {

  }

  bool CKODIAddon_InterProcess_Addon_VFSUtils::FileExists(const std::string& strFileName, bool bUseCache)
  {

  }

  int CKODIAddon_InterProcess_Addon_VFSUtils::StatFile(const std::string& strFileName, struct __stat64* buffer)
  {

  }

  bool CKODIAddon_InterProcess_Addon_VFSUtils::DeleteFile(const std::string& strFileName)
  {

  }

}; /* extern "C" */
