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

#include "InterProcess.h"
#include KITINCLUDE(ADDON_API_LEVEL, addon/VFSUtils.hpp)

#include <cstring>

API_NAMESPACE

namespace KodiAPI
{
namespace AddOn
{

  CVFSDirEntry::CVFSDirEntry(
        const std::string& label,
        const std::string& path,
        bool               bFolder,
        int64_t            size)
    : m_label(label),
      m_path(path),
      m_bFolder(bFolder),
      m_size(size)
  {
  }

  CVFSDirEntry::CVFSDirEntry(const VFSDirEntry& dirEntry)
    : m_label(dirEntry.label ? dirEntry.label : ""),
      m_path(dirEntry.path   ? dirEntry.path  : ""),
      m_bFolder(dirEntry.folder),
      m_size(dirEntry.size)
  {
  }

  const std::string& CVFSDirEntry::Label(void) const
  {
    return m_label;
  }

  const std::string& CVFSDirEntry::Path(void) const
  {
    return m_path;
  }

  bool CVFSDirEntry::IsFolder(void) const
  {
    return m_bFolder;
  }

  int64_t CVFSDirEntry::Size(void) const
  {
    return m_size;
  }

  void CVFSDirEntry::SetLabel(const std::string& label)
  {
    m_label = label;
  }

  void CVFSDirEntry::SetPath(const std::string& path)
  {
    m_path = path;
  }

  void CVFSDirEntry::SetFolder(bool bFolder)
  {
    m_bFolder = bFolder;
  }

  void CVFSDirEntry::SetSize(int64_t size)
  {
    m_size = size;
  }
  //----------------------------------------------------------------------------


  //============================================================================
  CVFSFile::CVFSFile()
   : m_pFile(nullptr)
  {

  }

  CVFSFile::~CVFSFile()
  {
    Close();
  }

  bool CVFSFile::OpenFile(const std::string& strFileName, unsigned int flags)
  {
    Close();
    m_pFile = g_interProcess.m_Callbacks->File.open_file(g_interProcess.m_Handle, strFileName.c_str(), flags);
    return m_pFile != nullptr;
  }

  bool CVFSFile::OpenFileForWrite(const std::string& strFileName, bool bOverWrite)
  {
    Close();

    // Try to open the file. If it fails, check if we need to create the directory first
    // This way we avoid checking if the directory exists every time
    m_pFile = g_interProcess.m_Callbacks->File.open_file_for_write(g_interProcess.m_Handle, strFileName.c_str(), bOverWrite);
    if (!m_pFile)
    {
      std::string cacheDirectory = KodiAPI::AddOn::VFSUtils::GetDirectoryName(strFileName);
      if (g_interProcess.m_Callbacks->Directory.directory_exists(g_interProcess.m_Handle, cacheDirectory.c_str()) ||
          g_interProcess.m_Callbacks->Directory.create_directory(g_interProcess.m_Handle, cacheDirectory.c_str()))
        m_pFile = g_interProcess.m_Callbacks->File.open_file_for_write(g_interProcess.m_Handle, strFileName.c_str(), bOverWrite);
    }
    return m_pFile != nullptr;
  }

  ssize_t CVFSFile::Read(void* lpBuf, size_t uiBufSize)
  {
    if (!m_pFile)
      return 0;
    return g_interProcess.m_Callbacks->File.read_file(g_interProcess.m_Handle, m_pFile, lpBuf, uiBufSize);
  }

  bool CVFSFile::ReadLine(std::string &strLine)
  {
    strLine.clear();
    if (!m_pFile)
      return false;
    // TODO: Read 1024 chars into buffer. If file position advanced that many
    // chars, we didn't hit a newline. Otherwise, if file position is 1 or 2
    // past the number of bytes read, we read (and skipped) a newline sequence.
    char buffer[1025];
    if (g_interProcess.m_Callbacks->File.read_file_string(g_interProcess.m_Handle, m_pFile, buffer, sizeof(buffer)))
    {
      strLine = buffer;
      return !strLine.empty();
    }
    return false;
  }

  ssize_t CVFSFile::Write(const void* lpBuf, size_t uiBufSize)
  {
    if (!m_pFile)
      return 0;
    return g_interProcess.m_Callbacks->File.write_file(g_interProcess.m_Handle, m_pFile, lpBuf, uiBufSize);
  }

  void CVFSFile::Flush()
  {
    if (!m_pFile)
      return;
    g_interProcess.m_Callbacks->File.flush_file(g_interProcess.m_Handle, m_pFile);
  }

  int64_t CVFSFile::Seek(int64_t iFilePosition, int iWhence)
  {
    if (!m_pFile)
      return 0;
    return g_interProcess.m_Callbacks->File.seek_file(g_interProcess.m_Handle, m_pFile, iFilePosition, iWhence);
  }

  int CVFSFile::Truncate(int64_t iSize)
  {
    if (!m_pFile)
      return -1;
    return g_interProcess.m_Callbacks->File.truncate_file(g_interProcess.m_Handle, m_pFile, iSize);
  }

  int64_t CVFSFile::GetPosition()
  {
    if (!m_pFile)
      return -1;
    return g_interProcess.m_Callbacks->File.get_file_position(g_interProcess.m_Handle, m_pFile);
  }

  int64_t CVFSFile::GetLength()
  {
    if (!m_pFile)
      return 0;
    return g_interProcess.m_Callbacks->File.get_file_length(g_interProcess.m_Handle, m_pFile);
  }

  void CVFSFile::Close()
  {
    if (!m_pFile)
      return;
    g_interProcess.m_Callbacks->File.close_file(g_interProcess.m_Handle, m_pFile);
    m_pFile = nullptr;
  }

  int CVFSFile::GetChunkSize()
  {
    if (!m_pFile)
      return 0;
    return g_interProcess.m_Callbacks->File.get_file_chunk_size(g_interProcess.m_Handle, m_pFile);
  }
  //----------------------------------------------------------------------------


  //============================================================================
  namespace VFSUtils
  {

    bool CreateDirectory(const std::string& strPath)
    {
      return g_interProcess.m_Callbacks->Directory.create_directory(g_interProcess.m_Handle, strPath.c_str());
    }

    bool DirectoryExists(const std::string& strPath)
    {
      return g_interProcess.m_Callbacks->Directory.directory_exists(g_interProcess.m_Handle, strPath.c_str());
    }

    bool RemoveDirectory(const std::string& strPath)
    {
      return g_interProcess.m_Callbacks->Directory.remove_directory(g_interProcess.m_Handle, strPath.c_str());
    }

    bool GetDirectory(
          const std::string&          path,
          const std::string&          mask,
          std::vector<CVFSDirEntry>&  items)
    {
      VFSDirEntry* dir_list  = nullptr;
      unsigned int num_items = 0;
      if (g_interProcess.m_Callbacks->VFS.get_directory(g_interProcess.m_Handle, path.c_str(), mask.c_str(), &dir_list, &num_items))
      {
        if (dir_list)
        {
          for (unsigned int i = 0; i < num_items; ++i)
            items.push_back(CVFSDirEntry(dir_list[i]));

          g_interProcess.m_Callbacks->VFS.free_directory(g_interProcess.m_Handle, dir_list, num_items);
        }

        return true;
      }
      return false;
    }

    std::string MakeLegalFileName(const std::string& strFileName)
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

    std::string MakeLegalPath(const std::string& strPath)
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

    bool FileExists(const std::string& strFileName, bool bUseCache)
    {
      return g_interProcess.m_Callbacks->File.file_exists(g_interProcess.m_Handle, strFileName.c_str(), bUseCache);
    }

    int StatFile(const std::string& strFileName, struct __stat64* buffer)
    {
      return g_interProcess.m_Callbacks->File.stat_file(g_interProcess.m_Handle, strFileName.c_str(), buffer);
    }

    bool DeleteFile(const std::string& strFileName)
    {
      return g_interProcess.m_Callbacks->File.delete_file(g_interProcess.m_Handle, strFileName.c_str());
    }

    std::string GetFileMD5(const std::string& strPath)
    {
      std::string strReturn;
      char* strMd5 = g_interProcess.m_Callbacks->File.get_file_md5(g_interProcess.m_Handle, strPath.c_str());
      if (strMd5 != nullptr)
      {
        if (std::strlen(strMd5))
          strReturn = strMd5;
        g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strMd5);
      }
      return strReturn;
    }

    std::string GetCacheThumbName(const std::string& strFileName)
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

    unsigned int GetChunkSize(unsigned int chunk, unsigned int minimum)
    {
      if (chunk)
        return chunk * ((minimum + chunk - 1) / chunk);
      else
        return minimum;
    }

    std::string GetFileName(const std::string& strFileNameAndPath)
    {
      /* find the last slash */
      const size_t slash = strFileNameAndPath.find_last_of("/\\");
      return strFileNameAndPath.substr(slash+1);
    }

    std::string GetDirectoryName(const std::string &strFilePath)
    {
      // Will from a full filename return the directory the file resides in.
      // Keeps the final slash at end and possible |option=foo options.

      size_t iPosSlash = strFilePath.find_last_of("/\\");
      if (iPosSlash == std::string::npos)
        return ""; // No slash, so no path (ignore any options)

      size_t iPosBar = strFilePath.rfind('|');
      if (iPosBar == std::string::npos)
        return strFilePath.substr(0, iPosSlash + 1); // Only path

      return strFilePath.substr(0, iPosSlash + 1) + strFilePath.substr(iPosBar); // Path + options
    }

  } /* namespace VFSUtils */

} /* namespace AddOn */
} /* namespace KodiAPI */

END_NAMESPACE()
