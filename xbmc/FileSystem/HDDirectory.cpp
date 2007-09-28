
#include "stdafx.h"
#include "HDDirectory.h"
#include "../Util.h"
#include "../xbox/IoSupport.h"
#include "DirectoryCache.h"
#include "iso9660.h"

using namespace DIRECTORY;

CHDDirectory::CHDDirectory(void)
{}

CHDDirectory::~CHDDirectory(void)
{}

bool CHDDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{
  WIN32_FIND_DATA wfd;

  CStdString strPath=strPath1;
#ifndef _LINUX
  g_charsetConverter.utf8ToStringCharset(strPath);
#endif

  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath1);


  CStdString strRoot = strPath;
  CURL url(strPath);

  memset(&wfd, 0, sizeof(wfd));
  if (!CUtil::HasSlashAtEnd(strPath) )
#ifndef _LINUX  
    strRoot += "\\";
  strRoot.Replace("/", "\\");
#else
    strRoot += "/";
#endif
  if (CUtil::IsDVD(strRoot) && m_isoReader.IsScanned())
  {
    // Reset iso reader and remount or
    // we can't access the dvd-rom
    m_isoReader.Reset();

    CIoSupport::Dismount("Cdrom0");
    CIoSupport::RemapDriveLetter('D', "Cdrom0");
  }

  CStdString strSearchMask = strRoot;
#ifndef _LINUX
  strSearchMask += "*.*";
#else
  strSearchMask += "*";
#endif

  FILETIME localTime;
  CAutoPtrFind hFind ( FindFirstFile(strSearchMask.c_str(), &wfd));
  
  // on error, check if path exists at all, this will return true if empty folder
  if (!hFind.isValid()) 
      return Exists(strPath1);

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
#ifndef _LINUX
            g_charsetConverter.stringCharsetToUtf8(strLabel);
#endif
            CFileItem *pItem = new CFileItem(strLabel);
            pItem->m_strPath = strRoot;
            pItem->m_strPath += wfd.cFileName;
#ifndef _LINUX
            g_charsetConverter.stringCharsetToUtf8(pItem->m_strPath);
#endif
            pItem->m_bIsFolder = true;
            CUtil::AddSlashAtEnd(pItem->m_strPath);
            FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
            pItem->m_dateTime=localTime;

            vecCacheItems.Add(pItem);
            items.Add(new CFileItem(*pItem));
          }
        }
        else
        {
          CStdString strLabel=wfd.cFileName;
#ifndef _LINUX
          g_charsetConverter.stringCharsetToUtf8(strLabel);
#endif
          CFileItem *pItem = new CFileItem(strLabel);
          pItem->m_strPath = strRoot;
          pItem->m_strPath += wfd.cFileName;
#ifndef _LINUX
          g_charsetConverter.stringCharsetToUtf8(pItem->m_strPath);
#endif

          pItem->m_bIsFolder = false;
          pItem->m_dwSize = CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          pItem->m_dateTime=localTime;
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
#ifdef _XBOX
    // if we use AutoPtrHandle, this auto-closes
    FindClose((HANDLE)hFind); //should be closed
#endif
  }
  if (m_cacheDirectory)
    g_directoryCache.SetDirectory(strPath1, vecCacheItems);
  return true;
}

bool CHDDirectory::Create(const char* strPath)
{
  CStdString strPath1 = strPath;
#ifndef _LINUX
  g_charsetConverter.utf8ToStringCharset(strPath1);
#endif
  if (!CUtil::HasSlashAtEnd(strPath1))
#ifndef _LINUX  
    strPath1 += '\\';
#else
    strPath1 += '/';
#endif

#ifdef HAS_FTP_SERVER
  // okey this is really evil, since the create will succed
  // caller have no idea that a different directory was created
  if (g_guiSettings.GetBool("servers.ftpautofatx"))
  {
    CStdString strPath2(strPath1);
    CUtil::GetFatXQualifiedPath(strPath1);
    if(strPath2 != strPath1)
      CLog::Log(LOGNOTICE,"fatxq: %s -> %s",strPath2.c_str(), strPath1.c_str());
  }
#endif

  if(::CreateDirectory(strPath1.c_str(), NULL))
    return true;
  else if(GetLastError() == ERROR_ALREADY_EXISTS)
    return true;

  return false;
}

bool CHDDirectory::Remove(const char* strPath)
{
  CStdString strPath1 = strPath;
#ifndef _LINUX
  g_charsetConverter.utf8ToStringCharset(strPath1);
#endif
  return ::RemoveDirectory(strPath1) ? true : false;
}

bool CHDDirectory::Exists(const char* strPath)
{
  CStdString strReplaced=strPath;
#ifndef _LINUX
  g_charsetConverter.utf8ToStringCharset(strReplaced);
  strReplaced.Replace("/","\\");
  CUtil::GetFatXQualifiedPath(strReplaced);
  if (!CUtil::HasSlashAtEnd(strReplaced))
    strReplaced += '\\';
#endif    
  DWORD attributes = GetFileAttributes(strReplaced.c_str());
  if (FILE_ATTRIBUTE_DIRECTORY == attributes) return true;
  return false;
}
