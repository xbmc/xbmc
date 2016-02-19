#pragma once
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

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_Addon_VFSUtils
  {
    bool CreateDirectory(const std::string& strPath);
    bool DirectoryExists(const std::string& strPath);
    bool RemoveDirectory(const std::string& strPath);
    bool GetVFSDirectory(const char *strPath, const char* mask, VFSDirEntry** items, unsigned int* num_items);
    void FreeVFSDirectory(VFSDirEntry* items, unsigned int num_items);
    std::string GetFileMD5(const std::string& strFileName);
    std::string GetCacheThumbName(const std::string& strFileName);
    std::string MakeLegalFileName(const std::string& strFileName);
    std::string MakeLegalPath(const std::string& strPath);
    void* OpenFile(const std::string& strFileName, unsigned int flags);
    void* OpenFileForWrite(const std::string& strFileName, bool bOverWrite);
    ssize_t ReadFile(void* file, void* lpBuf, size_t uiBufSize);
    bool ReadFileString(void* file, char *szLine, int iLineLength);
    ssize_t WriteFile(void* file, const void* lpBuf, size_t uiBufSize);
    void FlushFile(void* file);
    int64_t SeekFile(void* file, int64_t iFilePosition, int iWhence);
    int TruncateFile(void* file, int64_t iSize);
    int64_t GetFilePosition(void* file);
    int64_t GetFileLength(void* file);
    void CloseFile(void* file);
    int GetFileChunkSize(void* file);
    bool FileExists(const std::string& strFileName, bool bUseCache);
    int StatFile(const std::string& strFileName, struct __stat64* buffer);
    bool DeleteFile(const std::string& strFileName);
  };

}; /* extern "C" */
