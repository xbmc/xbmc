#include "../stdafx.h"
#include "ftpdirectory.h"
#include "ftpparse.h"
#include "../url.h"
#include "../util.h"
#include "DirectoryCache.h"
#include "FileCurl.h"


CFTPDirectory::CFTPDirectory(void){}
CFTPDirectory::~CFTPDirectory(void){}

bool CFTPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  g_directoryCache.ClearDirectory(strPath);
  CFileItemList vecCacheItems;

  CFileCurl reader;

  CStdString path = strPath;
  if( !path.Right(1).Equals("/") )
    path += "/";

  if (!reader.Open(path, false))
    return false;


  char buffer[MAX_PATH + 1024];
  while( reader.ReadString(buffer, sizeof(buffer)) )
  {
    CStdString strBuffer = buffer;

    StringUtils::RemoveCRLF(strBuffer);

    struct ftpparse lp = {};
    if (ftpparse(&lp, (char*)strBuffer.c_str(), strBuffer.size()) == 1)
    {
      if( lp.namelen == 0 )
        continue;

      if( lp.flagtrycwd == 0 && lp.flagtryretr == 0 )
        continue;

      /* buffer name as it's not allways null terminated */
      CStdString name, filename;
      name.assign(lp.name, lp.namelen);
      filename.assign(lp.name, lp.namelen);

      if( name.Equals("..") || name.Equals(".") )
        continue;

      /* this should be conditional if we ever add    */
      /* support for the utf8 extension in ftp client */
      g_charsetConverter.stringCharsetToUtf8(name);

      /* keep it in whatever format it was as we do no */
      /* charset conversion in curl client currently   */
      /* just make sure it's url encoded properly      */
      /* TODO, could we finally fix so all our urls are encoded. grr */
      g_charsetConverter.stringCharsetToUtf8(filename);
      //CUtil::URLEncode(filename);

      CFileItem* pItem = new CFileItem(name);
      pItem->m_strPath = path + filename;
      pItem->m_bIsFolder = (bool)(lp.flagtrycwd != 0);
      pItem->m_dwSize = lp.size;
      pItem->m_dateTime=lp.mtime;

      if( m_cacheDirectory )
        vecCacheItems.Add(new CFileItem(*pItem));

      /* if file is ok by mask or a folder add it */
      if( pItem->m_bIsFolder || IsAllowed(name) )
        items.Add(pItem);
      else
        delete pItem;
    }
  }


 if (m_cacheDirectory)
    g_directoryCache.SetDirectory(path, vecCacheItems);
  return true;
}