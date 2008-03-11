
#include "stdafx.h"
#include "WIN32WNetDirectory.h"
#include "../Util.h"
#include "../xbox/IoSupport.h"
#include "FileSystem/DirectoryCache.h"
#include "FileSystem/iso9660.h"
#include "../GUIPassword.h"
#include "WIN32Util.h"

using namespace AUTOPTR;
using namespace DIRECTORY;

CWNetDirectory::CWNetDirectory(void)
{
  strMntPoint = "";
}

CWNetDirectory::~CWNetDirectory(void)
{
  WNetCancelConnection2((LPCTSTR)strMntPoint.c_str(),NULL,NULL);
}

bool CWNetDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{

  WIN32_FIND_DATA wfd;

  CStdString strPath=strPath1;

  g_charsetConverter.utf8ToStringCharset(strPath);

  if(strMntPoint=="")
    strMntPoint = CWIN32Util::MountShare(strPath);

  if(strMntPoint=="")
    return false;

  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath1);


  CStdString strRoot = strMntPoint;
  CURL url(strMntPoint);

  memset(&wfd, 0, sizeof(wfd));
  if (!CUtil::HasSlashAtEnd(strMntPoint) )
    strRoot += "\\";

  strRoot.Replace("/", "\\");


  CStdString strSearchMask = strRoot;

  strSearchMask += "*.*";

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

            g_charsetConverter.stringCharsetToUtf8(strLabel);

            CFileItem *pItem = new CFileItem(strLabel);
            pItem->m_strPath = strPath1;
            pItem->m_strPath += wfd.cFileName;
            g_charsetConverter.stringCharsetToUtf8(pItem->m_strPath);
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

          g_charsetConverter.stringCharsetToUtf8(strLabel);

          CFileItem *pItem = new CFileItem(strLabel);
          pItem->m_strPath = strPath1;
          pItem->m_strPath += wfd.cFileName;

          g_charsetConverter.stringCharsetToUtf8(pItem->m_strPath);

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
  }
  if (m_cacheDirectory)
    g_directoryCache.SetDirectory(strPath1, vecCacheItems);
  return true;
}

bool CWNetDirectory::Create(const char* strPath)
{
  CStdString strPath1 = strPath;

  g_charsetConverter.utf8ToStringCharset(strPath1);

  CURL url(strPath1);

  if (!CUtil::HasSlashAtEnd(strPath1))
    strPath1 += '\\';

  if(::CreateDirectory(strPath1.c_str(), NULL))
    return true;
  else if(GetLastError() == ERROR_ALREADY_EXISTS)
    return true;

  return false;
}

bool CWNetDirectory::Remove(const char* strPath)
{
  CStdString strPath1 = strPath;
  g_charsetConverter.utf8ToStringCharset(strPath1);
  return ::RemoveDirectory(strPath1) ? true : false;
}

bool CWNetDirectory::Exists(const char* strPath)
{
  CStdString strReplaced=strPath;
  g_charsetConverter.utf8ToStringCharset(strReplaced);
  strReplaced.Replace("/","\\");
  CUtil::GetFatXQualifiedPath(strReplaced);
  if (!CUtil::HasSlashAtEnd(strReplaced))
    strReplaced += '\\';
  
  DWORD attributes = GetFileAttributes(strReplaced.c_str());
  if (FILE_ATTRIBUTE_DIRECTORY == attributes) return true;
  return false;
}
