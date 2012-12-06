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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ZipDirectory.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "utils/URIUtils.h"
#include "Util.h"
#include "URL.h"
#include "ZipManager.h"
#include "FileItem.h"

#include <vector>

using namespace std;
namespace XFILE
{
  CZipDirectory::CZipDirectory()
  {
  }

  CZipDirectory::~CZipDirectory()
  {
  }

  bool CZipDirectory::GetDirectory(const CStdString& strPathOrig, CFileItemList& items)
  {
    CStdString strPath;

    /* if this isn't a proper archive path, assume it's the path to a archive file */
    if( !strPathOrig.Left(6).Equals("zip://") )
      URIUtils::CreateArchivePath(strPath, "zip", strPathOrig, "");
    else
      strPath = strPathOrig;

    CURL url(strPath);

    CStdString strArchive = url.GetHostName();
    CStdString strOptions = url.GetOptions();
    CStdString strPathInZip = url.GetFileName();

    url.SetOptions(""); // delete options to have a clean path to add stuff too
    url.SetFileName(""); // delete filename too as our names later will contain it

    CStdString strSlashPath = url.Get();

    CStdString strBuffer;

    // the RAR code depends on things having a "/" at the end of the path
    URIUtils::AddSlashAtEnd(strSlashPath);

    vector<SZipEntry> entries;
    // turn on fast lookups
    bool bWasFast(items.GetFastLookup());
    items.SetFastLookup(true);
    if (!g_ZipManager.GetZipList(strPath,entries))
      return false;

    vector<CStdString> baseTokens;
    if (!strPathInZip.IsEmpty())
      CUtil::Tokenize(strPathInZip,baseTokens,"/");

    for (vector<SZipEntry>::iterator ze=entries.begin();ze!=entries.end();++ze)
    {
      CStdString strEntryName(ze->name);
      strEntryName.Replace('\\','/');
      if (strEntryName == strPathInZip) // skip the listed dir
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
      char c=ze->name[strEntryName.size()];
      if (c == '/' || c == '\\')
        strEntryName += '/';
      bool bIsFolder = false;
      if (strEntryName[strEntryName.size()-1] != '/') // this is a file
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
      {
        pFileItem->m_dwSize = 0;
        URIUtils::AddSlashAtEnd(strBuffer);
      }
      else
        pFileItem->m_dwSize = ze->usize;
      pFileItem->SetPath(strBuffer);
      pFileItem->m_bIsFolder = bIsFolder;
      pFileItem->m_idepth = ze->method;
      items.Add(pFileItem);
    }
    items.SetFastLookup(bWasFast);
    return true;
  }

  bool CZipDirectory::ContainsFiles(const CStdString& strPath)
  {
    vector<SZipEntry> items;
    g_ZipManager.GetZipList(strPath,items);
    if (items.size())
    {
      if (items.size() > 1)
        return true;

      return false;
    }

    return false;
  }
}

