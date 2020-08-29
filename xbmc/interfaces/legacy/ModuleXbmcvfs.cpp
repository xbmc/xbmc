/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ModuleXbmcvfs.h"

#include "LanguageHook.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"

namespace XBMCAddon
{

  namespace xbmcvfs
  {
    bool copy(const String& strSource, const String& strDestination)
    {
      DelayedCallGuard dg;
      return XFILE::CFile::Copy(strSource, strDestination);
    }

    // delete a file
    bool deleteFile(const String& strSource)
    {
      DelayedCallGuard dg;
      return XFILE::CFile::Delete(strSource);
    }

    // rename a file
    bool rename(const String& file, const String& newFile)
    {
      DelayedCallGuard dg;
      return XFILE::CFile::Rename(file,newFile);
    }

    // check for a file or folder existence, mimics Pythons os.path.exists()
    bool exists(const String& path)
    {
      DelayedCallGuard dg;
      if (URIUtils::HasSlashAtEnd(path, true))
        return XFILE::CDirectory::Exists(path, false);
      return XFILE::CFile::Exists(path, false);
    }

    // make legal file name
    String makeLegalFilename(const String& filename)
    {
      XBMC_TRACE;
      return CUtil::MakeLegalPath(filename);
    }

    // translate path
    String translatePath(const String& path)
    {
      XBMC_TRACE;
      return CSpecialProtocol::TranslatePath(path);
    }

    // validate path
    String validatePath(const String& path)
    {
      XBMC_TRACE;
      return CUtil::ValidatePath(path, true);
    }

    // make a directory
    bool mkdir(const String& path)
    {
      DelayedCallGuard dg;
      return XFILE::CDirectory::Create(path);
    }

    // make all directories along the path
    bool mkdirs(const String& path)
    {
      DelayedCallGuard dg;
      return CUtil::CreateDirectoryEx(path);
    }

    bool rmdir(const String& path, bool force)
    {
      DelayedCallGuard dg;

      if (force)
        return CFileUtils::DeleteItem(path);
      else
        return XFILE::CDirectory::Remove(path);
    }

    Tuple<std::vector<String>, std::vector<String> > listdir(const String& path)
    {
      DelayedCallGuard dg;
      CFileItemList items;
      std::string strSource;
      strSource = path;
      XFILE::CDirectory::GetDirectory(strSource, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS);

      Tuple<std::vector<String>, std::vector<String> > ret;
      // initialize the Tuple to two values
      ret.second();

      for (int i=0; i < items.Size(); i++)
      {
        std::string itemPath = items[i]->GetPath();

        if (URIUtils::HasSlashAtEnd(itemPath)) // folder
        {
          URIUtils::RemoveSlashAtEnd(itemPath);
          std::string strFileName = URIUtils::GetFileName(itemPath);
          if (strFileName.empty())
          {
            CURL url(itemPath);
            strFileName = url.GetHostName();
          }
          ret.first().push_back(strFileName);
        }
        else // file
        {
          std::string strFileName = URIUtils::GetFileName(itemPath);
          ret.second().push_back(strFileName);
        }
      }

      return ret;
    }
  }
}
