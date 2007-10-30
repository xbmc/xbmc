#include "stdafx.h"
#include "ZipDirectory.h"
#include "../utils/log.h"
#include "../Util.h"
#include "../lib/zlib/zlib.h"

#include <vector>

namespace DIRECTORY
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
      CUtil::CreateZipPath(strPath, strPathOrig, "");
    else
      strPath = strPathOrig;

    CURL url(strPath);

    CStdString strArchive = url.GetHostName();
    CStdString strOptions = url.GetOptions();
    CStdString strPathInZip = url.GetFileName();

    url.SetOptions(""); // delete options to have a clean path to add stuff too
    url.SetFileName(""); // delete filename too as our names later will contain it

    CStdString strSlashPath;
    url.GetURL(strSlashPath);

    CStdString strBuffer;

    // the RAR code depends on things having a "/" at the end of the path
    if (!CUtil::HasSlashAtEnd(strSlashPath))
      strSlashPath += '/';

    std::vector<SZipEntry> entries;
    // turn on fast lookups
    bool bWasFast(items.GetFastLookup());
    items.SetFastLookup(true);
    if (!g_ZipManager.GetZipList(strPath,entries))
      return false;

    CStdString strSkip;
    std::vector<CStdString> baseTokens;
    if (!strPathInZip.IsEmpty())
      CUtil::Tokenize(strPathInZip,baseTokens,"/");

    for (std::vector<SZipEntry>::iterator ze=entries.begin();ze!=entries.end();++ze)
    {      
      CStdString strEntryName(ze->name);
      strEntryName.Replace('\\','/');
      if (strEntryName == strPathInZip) // skip the listed dir
        continue; 

      std::vector<CStdString> pathTokens;
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
        if (!IsAllowed(pathTokens[pathTokens.size()-1])) // not allowed
          continue;
  
        strBuffer = strSlashPath + strEntryName + strOptions;

      }
      else
      { // this is new folder. add if not already added
        bIsFolder = true;
        strBuffer = strSlashPath + strEntryName + strOptions;
        if (items.Contains(strBuffer)) // already added
          continue;
      }

      CFileItem* pFileItem = new CFileItem;
      pFileItem->SetLabel(pathTokens[baseTokens.size()]);
      if (bIsFolder)
        pFileItem->m_dwSize = 0;
      else
        pFileItem->m_dwSize = ze->usize;
      pFileItem->m_strPath = strBuffer;
      pFileItem->m_bIsFolder = bIsFolder;
      if (bIsFolder)
        CUtil::AddSlashAtEnd(pFileItem->m_strPath);
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
