#include "../stdafx.h"
#include "ZipDirectory.h"
#include "../application.h"
#include "../utils/log.h"
#include "../util.h"
#include "zlib.h"

#include <vector>

// TODO: enable in my videos, fix bug with cbz in file manager, enable in my music, filezip::stat

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
    baseTokens.push_back("krake"); // need one extra.. don't give a damn 'bout what it is
    for (std::vector<SZipEntry>::iterator ze=entries.begin();ze!=entries.end();++ze)
    {      
      CStdString strEntryName(ze->name);
      strEntryName.Replace("/","\\");
      if (strEntryName == url.GetFileName()) // skip the listed dir
        continue; 
      std::vector<CStdString> pathTokens;
      CUtil::Tokenize(strEntryName,pathTokens,"\\");
      if (pathTokens.size() != baseTokens.size())
        continue;
      bool bAdd=true;
      for ( unsigned int i=0;i<baseTokens.size()-1;++i )
      {
        if (pathTokens[i] != baseTokens[i])
        {
          bAdd = false;
          break;
        }
      }
      if (!bAdd)
        continue;

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
      pFileItem->SetLabel(pathTokens[pathTokens.size()-1]);
      pFileItem->m_dwSize = ze->usize;
      pFileItem->m_strPath = strEntryName;
      pFileItem->m_bIsFolder = bIsFolder;
      items.Add(pFileItem);
    }
    items.SetFastLookup(bWasFast);
    return true;
  }
}