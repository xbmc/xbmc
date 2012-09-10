/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "ModuleXbmcvfs.h"
#include "LanguageHook.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "Util.h"

namespace XBMCAddon
{

  namespace xbmcvfs
  {
    bool copy(const String& strSource, const String& strDestnation)
    {
      DelayedCallGuard dg;
      return XFILE::CFile::Cache(strSource, strDestnation);
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

    // check for a file or folder existance, mimics Pythons os.path.exists()
    bool exists(const String& path)
    {
      DelayedCallGuard dg;
      return XFILE::CFile::Exists(path, false);
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
      return (force ? CFileUtils::DeleteItem(path,force) : XFILE::CDirectory::Remove(path));
    }      

    Tuple<std::vector<String>, std::vector<String> > listdir(const String& path)
    {
      CFileItemList items;
      CStdString strSource;
      strSource = path;
      XFILE::CDirectory::GetDirectory(strSource, items);

      Tuple<std::vector<String>, std::vector<String> > ret;
      // initialize the Tuple to two values
      ret.second();

      for (int i=0; i < items.Size(); i++)
      {
        CStdString itemPath = items[i]->GetPath();
        
        if (URIUtils::HasSlashAtEnd(itemPath)) // folder
        {
          URIUtils::RemoveSlashAtEnd(itemPath);
          CStdString strFileName = URIUtils::GetFileName(itemPath);
          ret.first().push_back(strFileName);
        }
        else // file
        {
          CStdString strFileName = URIUtils::GetFileName(itemPath);
          ret.second().push_back(strFileName);
        }
      }

      return ret;
    }
  }
}
