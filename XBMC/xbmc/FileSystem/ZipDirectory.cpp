#include "../stdafx.h"
#include "ZipDirectory.h"
#include "../utils/log.h"
#include "../util.h"
#include "zlib.h"

#include <vector>

namespace DIRECTORY
{
  CZipDirectory::CZipDirectory()
  {
  }

  CZipDirectory::~CZipDirectory()
  {
  }

  bool CZipDirectory::GetDirectory(const CStdString& strPath, CFileItemList& items)
  {    
    CURL url(strPath);
    std::vector<SZipEntry> entries;
    // turn on fast lookups
    bool bWasFast(items.GetFastLookup());
    items.SetFastLookup(true);
    if (!g_ZipManager.GetZipList(strPath,entries))
      return false;
    CStdString strSkip;
    std::vector<CStdString> baseTokens;
    if (!url.GetFileName().IsEmpty())
      CUtil::Tokenize(url.GetFileName(),baseTokens,"/\\");
    for (std::vector<SZipEntry>::iterator ze=entries.begin();ze!=entries.end();++ze)
    {      
      CStdString strEntryName(ze->name);
      strEntryName.Replace("/","\\");
      if (strEntryName == url.GetFileName()) // skip the listed dir
        continue; 
      std::vector<CStdString> pathTokens;
      CUtil::Tokenize(strEntryName,pathTokens,"\\");
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
        strEntryName += pathTokens[i]+"\\";
      }
      if (!bAdd)
        continue;

      strEntryName += pathTokens[baseTokens.size()];
      char c=ze->name[strEntryName.size()];
      if (c == '/' || c == '\\')
        strEntryName += '\\';
      bool bIsFolder = false;
      if (strEntryName[strEntryName.size()-1] != '\\') // this is a file
      {
        if (!IsAllowed(pathTokens[pathTokens.size()-1])) // not allowed
          continue;

        strEntryName.Format("zip://%s,%i,%s,%s,\\%s",url.GetDomain().c_str(),url.GetPort(),url.GetPassWord().c_str(),url.GetHostName().c_str(),strEntryName.c_str());
      }
      else
      { // this is new folder. add if not already added
        bIsFolder = true;
        strEntryName.Format("zip://%s,%i,%s,%s,\\%s",url.GetDomain().c_str(),url.GetPort(),url.GetPassWord().c_str(),url.GetHostName().c_str(),strEntryName.c_str());
        if (items.Contains(strEntryName)) // already added
          continue;
      }
      CFileItem* pFileItem = new CFileItem;
      pFileItem->SetLabel(pathTokens[baseTokens.size()]);
      if (bIsFolder)
        pFileItem->m_dwSize = 0;
      else
        pFileItem->m_dwSize = ze->usize;
      pFileItem->m_strPath = strEntryName;
      pFileItem->m_bIsFolder = bIsFolder;
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