/*
 *      Copyright (C) 2011 Team XBMC
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

#include "ArchiveDirectory.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "utils/URIUtils.h"
#include "Util.h"
#include "URL.h"
#include "ArchiveManager.h"
#include "FileItem.h"

#include <deque>
#include <sys/stat.h>

using namespace std;
namespace XFILE
{
  CArchiveDirectory::CArchiveDirectory(){}

  CArchiveDirectory::~CArchiveDirectory(){}

  bool CArchiveDirectory::GetDirectory(const CStdString& strPathOrig, CFileItemList& items)
  {
    CStdString strPath;

    /* if this isn't a proper archive path, assume it's the path to a archive file */
    if( !strPathOrig.Left(10).Equals("archive://") )
      URIUtils::CreateArchivePath(strPath, "archive", strPathOrig, "");
    else
      strPath = strPathOrig;

    CURL url(strPath);

    CStdString strArchive = url.GetHostName();
    CStdString strOptions = url.GetOptions();
    CStdString strPathInArchive = url.GetFileName();

    url.SetOptions(""); // delete options to have a clean path to add stuff too
    url.SetFileName(""); // delete filename too as our names later will contain it

    CStdString strSlashPath = url.Get();

    CStdString strBuffer;

    deque<CArchiveEntry> entries;
    // turn on fast lookups
    bool bWasFast(items.GetFastLookup());
    items.SetFastLookup(true);
    if (!g_archiveManager.GetArchiveList(strPath,entries))
      return false;

    vector<CStdString> baseTokens;
    if (!strPathInArchive.IsEmpty())
      CUtil::Tokenize(strPathInArchive,baseTokens,"/");

    for (deque<CArchiveEntry>::iterator it = entries.begin(); it != entries.end(); ++it)
    {
      CStdString strEntryName;
      it->get_file(strEntryName);
      strEntryName.Replace('\\','/');
      if (strEntryName == strPathInArchive) // skip the listed dir
        continue;

      vector<CStdString> pathTokens;
      CUtil::Tokenize(strEntryName,pathTokens,"/");
      if (pathTokens.size() < baseTokens.size()+1)
        continue;

      bool bAdd=true;
      strEntryName = "";
      for ( unsigned int i=0;i<baseTokens.size();++i )
      {
        if (pathTokens[i] != baseTokens[i])
        {
          bAdd = false;
          break;
        }
        strEntryName += pathTokens[i] + "/";
      }
      if (!bAdd)
        continue;

      strEntryName += pathTokens[baseTokens.size()];
      char c=strEntryName.c_str()[strEntryName.size()];
      if (c == '/' || c == '\\')
        strEntryName += '/';
      bool bIsFolder = false;
      const struct stat *file_stat = it->get_file_stat();
      if (!S_ISDIR(file_stat->st_mode)) // this is a file
      {
        strBuffer = strSlashPath + strEntryName + strOptions;
      }
      else
      { // this is new folder. add if not already added
        bIsFolder = true;
        strBuffer = strSlashPath + strEntryName + strOptions;
        if (items.Contains(strBuffer)) // already added
          continue;
      }

      CFileItemPtr pFileItem(new CFileItem);

      if (g_charsetConverter.isValidUtf8(pathTokens[baseTokens.size()]))
        g_charsetConverter.utf8ToStringCharset(pathTokens[baseTokens.size()]);

      pFileItem->SetLabel(pathTokens[baseTokens.size()]);
      if (bIsFolder)
        pFileItem->m_dwSize = 0;
      else
        pFileItem->m_dwSize = file_stat->st_size;
      pFileItem->m_strPath = strBuffer;
      pFileItem->m_bIsFolder = bIsFolder;
      if (bIsFolder)
        URIUtils::AddSlashAtEnd(pFileItem->m_strPath);
      items.Add(pFileItem);
    }
    items.SetFastLookup(bWasFast);
    return true;
  }

  bool CArchiveDirectory::ContainsFiles(const CStdString& strPath)
  {
    deque<CArchiveEntry> items;
    g_archiveManager.GetArchiveList(strPath,items);
    if (!items.empty())
      return true;
    return false;
  }
}

