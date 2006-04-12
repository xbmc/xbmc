
#include "../stdafx.h"
#include "hddirectory.h"
#include "../util.h"
#include "../xbox/iosupport.h"
#include "directorycache.h"
#include "iso9660.h"


CHDDirectory::CHDDirectory(void)
{}

CHDDirectory::~CHDDirectory(void)
{}

bool CHDDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{
  WIN32_FIND_DATA wfd;

  CStdString strPath=strPath1;
  g_charsetConverter.utf8ToStringCharset(strPath);

  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath1);


  CStdString strRoot = strPath;
  CURL url(strPath);

  memset(&wfd, 0, sizeof(wfd));
  if (!CUtil::HasSlashAtEnd(strPath) )
    strRoot += "\\";
  strRoot.Replace("/", "\\");
  if (CUtil::IsDVD(strRoot) && m_isoReader.IsScanned())
  {
    // Reset iso reader and remount or
    // we can't access the dvd-rom
    m_isoReader.Reset();
    CIoSupport helper;
    helper.Remount("D:", "Cdrom0");
  }

  CStdString strSearchMask = strRoot;
  strSearchMask += "*.*";

  FILETIME localTime;
  CAutoPtrFind hFind ( FindFirstFile(strSearchMask.c_str(), &wfd));
  // Causing: Empty Folder not openable!
  //if (!hFind.isValid()) return false;
  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
          CStdString strDir = wfd.cFileName;
          if (strDir != "." && strDir != "..")
          {
            CStdString strLabel=wfd.cFileName;
            g_charsetConverter.stringCharsetToUtf8(strLabel);
            CFileItem *pItem = new CFileItem(strLabel);
            pItem->m_strPath = strRoot;
            pItem->m_strPath += wfd.cFileName;
            g_charsetConverter.stringCharsetToUtf8(pItem->m_strPath);
            pItem->m_bIsFolder = true;
            FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
            FileTimeToSystemTime(&localTime, &pItem->m_stTime);

            vecCacheItems.Add(pItem);
            items.Add(new CFileItem(*pItem));
          }
        }
        else
        {
          CStdString strLabel=wfd.cFileName;
          g_charsetConverter.stringCharsetToUtf8(strLabel);
          CFileItem *pItem = new CFileItem(strLabel);
          pItem->m_strPath = strRoot;
          pItem->m_strPath += wfd.cFileName;
          g_charsetConverter.stringCharsetToUtf8(pItem->m_strPath);

          pItem->m_bIsFolder = false;
          pItem->m_dwSize = CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          FileTimeToSystemTime(&localTime, &pItem->m_stTime);
          if ( IsAllowed( wfd.cFileName) )
          {
            vecCacheItems.Add(pItem);
            items.Add(new CFileItem(*pItem));
          }
          else
            vecCacheItems.Add(pItem);
        }
      }
    }
    while (FindNextFile((HANDLE)hFind, &wfd));
    FindClose((HANDLE)hFind); //should be closed
  }
  if (m_cacheDirectory)
    g_directoryCache.SetDirectory(strPath1, vecCacheItems);
  return true;
}

bool CHDDirectory::Create(const char* strPath)
{
  CStdString strPath1 = strPath;
  g_charsetConverter.utf8ToStringCharset(strPath1);
  if (!CUtil::HasSlashAtEnd(strPath1))
    strPath1 += '\\';
  if (g_guiSettings.GetBool("Servers.FTPAutoFatX"))
    CUtil::GetFatXQualifiedPath(strPath1);
  CLog::Log(LOGDEBUG,"fatxq: %s",strPath1.c_str());
  return ::CreateDirectory(strPath1.c_str(), NULL) ? true : false;
}

bool CHDDirectory::Remove(const char* strPath)
{
  CStdString strPath1 = strPath;
  g_charsetConverter.utf8ToStringCharset(strPath1);
  return ::RemoveDirectory(strPath1) ? true : false;
}

bool CHDDirectory::Exists(const char* strPath)
{
  CStdString strReplaced=strPath;
  g_charsetConverter.utf8ToStringCharset(strReplaced);
  strReplaced.Replace("/","\\");
  if (!CUtil::HasSlashAtEnd(strReplaced))
    strReplaced += '\\';
  CUtil::GetFatXQualifiedPath(strReplaced);
  DWORD attributes = GetFileAttributes(strReplaced);
  if (FILE_ATTRIBUTE_DIRECTORY == attributes) return true;
  return false;
}
