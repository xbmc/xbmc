/*
 *      Copyright (C) 2005-2008 Team XBMC
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


#include "stdafx.h"
#include "WINSMBDirectory.h"
#include "Util.h"
#include "DirectoryCache.h"
#include "URL.h"
#include "GUISettings.h"
#include "FileItem.h"
#include "WNETHelper.h"
#include "WIN32Util.h"

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD) -1)
#endif


using namespace AUTOPTR;
using namespace DIRECTORY;

CWINSMBDirectory::CWINSMBDirectory(void)
{
  m_strHost.clear();
}

CWINSMBDirectory::~CWINSMBDirectory(void)
{
  m_strHost.clear();
}

bool CWINSMBDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{
  WIN32_FIND_DATAW wfd;

  CStdString strPath=strPath1;

  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath1);


  CStdString strRoot = strPath;
  CURL url(strPath);

  if(url.GetShareName().empty())
  {
    LPNETRESOURCE lpnr = NULL;
    bool ret;
    if(!url.GetHostName().empty())
    {
      lpnr = (LPNETRESOURCE) GlobalAlloc(GPTR, 16384);
      if(lpnr == NULL)
        return false;

      CStdString strHost = "\\\\" + url.GetHostName();
      lpnr->lpRemoteName = (char*)strHost.c_str();
      m_strHost = url.GetHostName();
      ret = EnumerateFunc(lpnr, items);
      GlobalFree((HGLOBAL) lpnr);
      m_strHost.clear();
    }
    else
      ret = EnumerateFunc(lpnr, items);  
 
    return ret; 
  }

  memset(&wfd, 0, sizeof(wfd));
  strRoot.Replace("smb:","");
  if (!CUtil::HasSlashAtEnd(strPath) ) 
    strRoot += "/";
  strRoot.Replace("/", "\\");

  CStdStringW strSearchMask;
  g_charsetConverter.utf8ToW(strRoot, strSearchMask, false); 
  strSearchMask += "*.*";

  FILETIME localTime;
  CAutoPtrFind hFind ( FindFirstFileW(strSearchMask.c_str(), &wfd));
  
  // on error, check if path exists at all, this will return true if empty folder
  if (!hFind.isValid()) 
  {
    DWORD ret = GetLastError();
      return Exists(strPath1);
  }

  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        CStdString strLabel;
        g_charsetConverter.wToUTF8(wfd.cFileName,strLabel);
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
          if (strLabel != "." && strLabel != "..")
          {
            CFileItemPtr pItem(new CFileItem(strLabel));
            pItem->m_strPath = strPath;
            CUtil::AddSlashAtEnd(pItem->m_strPath);
            pItem->m_strPath += strLabel;
            pItem->m_bIsFolder = true;
            CUtil::AddSlashAtEnd(pItem->m_strPath);
            FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
            pItem->m_dateTime=localTime;

            vecCacheItems.Add(pItem);

            /* Checks if the file is hidden. If it is then we don't really need to add it */
            if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || g_guiSettings.GetBool("filelists.showhidden"))
              items.Add(pItem);
          }
        }
        else
        {
          CFileItemPtr pItem(new CFileItem(strLabel));
          pItem->m_strPath = strPath;
          CUtil::AddSlashAtEnd(pItem->m_strPath);
          pItem->m_strPath += strLabel;
          pItem->m_bIsFolder = false;
          pItem->m_dwSize = CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          pItem->m_dateTime=localTime;

          /* Checks if the file is hidden. If it is then we don't really need to add it */
          if ((!(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || g_guiSettings.GetBool("filelists.showhidden")) && IsAllowed(strLabel))
          {
            vecCacheItems.Add(pItem);
            items.Add(pItem);
          }
          else
            vecCacheItems.Add(pItem);
        }
      }
    }
    while (FindNextFileW((HANDLE)hFind, &wfd));
  }
  if (m_cacheDirectory)
    g_directoryCache.SetDirectory(strPath1, vecCacheItems);
  return true;
}

bool CWINSMBDirectory::Create(const char* strPath)
{
  CStdString strPath1 = strPath;
  if (!CUtil::HasSlashAtEnd(strPath1))
    strPath1 += '/';

  strPath1.Replace("smb:","");
  strPath1.Replace("/","\\");

  CStdStringW strWPath1;
  g_charsetConverter.utf8ToW(strPath1, strWPath1, false);
  if(::CreateDirectoryW(strWPath1, NULL))
    return true;
  else if(GetLastError() == ERROR_ALREADY_EXISTS)
    return true;

  return false;
}

bool CWINSMBDirectory::Remove(const char* strPath)
{
  CStdStringW strWPath;
  CStdString strPath1 = strPath;
  strPath1.Replace("smb:","");
  strPath1.Replace("/","\\");
  g_charsetConverter.utf8ToW(strPath1, strWPath, false);
  return ::RemoveDirectoryW(strWPath) ? true : false;
}

bool CWINSMBDirectory::Exists(const char* strPath)
{
  CStdString strReplaced=strPath;
  CStdStringW strWReplaced;
  strReplaced.Replace("smb:","");
  strReplaced.Replace("/","\\");
  if (!CUtil::HasSlashAtEnd(strReplaced))
    strReplaced += '\\';
  g_charsetConverter.utf8ToW(strReplaced, strWReplaced, false);
  DWORD attributes = GetFileAttributesW(strWReplaced);
  if(attributes == INVALID_FILE_ATTRIBUTES)
    return false;
  if (FILE_ATTRIBUTE_DIRECTORY & attributes) return true;
  return false;
}

bool CWINSMBDirectory::EnumerateFunc(LPNETRESOURCE lpnr, CFileItemList &items)
{
  DWORD dwResult, dwResultEnum;
  HANDLE hEnum;
  DWORD cbBuffer = 16384;     // 16K is a good size
  DWORD cEntries = -1;        // enumerate all possible entries
  LPNETRESOURCE lpnrLocal;    // pointer to enumerated structures
  DWORD i;
  //
  // Call the WNetOpenEnum function to begin the enumeration.
  //
  dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, // all network resources
                          RESOURCETYPE_ANY,   // all resources
                          0,  // enumerate all resources
                          lpnr,       // NULL first time the function is called
                          &hEnum);    // handle to the resource

  if (dwResult != NO_ERROR) 
  {
    CLog::Log(LOGERROR,"WnetOpenEnum failed with error %d\n", dwResult);
    return false;
  }
  //
  // Call the GlobalAlloc function to allocate resources.
  //
  lpnrLocal = (LPNETRESOURCE) GlobalAlloc(GPTR, cbBuffer);
  if (lpnrLocal == NULL) 
  {
    CLog::Log(LOGERROR,"WnetOpenEnum failed with error %d\n", dwResult);
    return false;
  }

  do 
  {
    //
    // Initialize the buffer.
    //
    ZeroMemory(lpnrLocal, cbBuffer);
    //
    // Call the WNetEnumResource function to continue
    //  the enumeration.
    //
    dwResultEnum = WNetEnumResource(hEnum,  // resource handle
                                    &cEntries,      // defined locally as -1
                                    lpnrLocal,      // LPNETRESOURCE
                                    &cbBuffer);     // buffer size
    //
    // If the call succeeds, loop through the structures.
    //
    if (dwResultEnum == NO_ERROR) 
    {
      for (i = 0; i < cEntries; i++) 
      {
        DWORD dwDisplayType = lpnrLocal[i].dwDisplayType;
        DWORD dwType = lpnrLocal[i].dwType;

        if(((dwDisplayType == RESOURCEDISPLAYTYPE_SERVER) || 
           (dwDisplayType == RESOURCEDISPLAYTYPE_SHARE)) &&
           (dwType != RESOURCETYPE_PRINT))
        {
          CStdString strurl = "smb:";
          CStdString strName = lpnrLocal[i].lpComment;
          CStdString strRemoteName = lpnrLocal[i].lpRemoteName;

          strurl.append(strRemoteName);
          strurl.Replace("\\","/");
          CURL rooturl(strurl);
          rooturl.SetFileName("");

          if(strName.empty())
          {
            if(!rooturl.GetShareName().empty())
              strName = rooturl.GetShareName();
            else
              strName = rooturl.GetHostName();

            strName.Replace("\\","");
          }

          CFileItemPtr pItem(new CFileItem(strName));        
          pItem->m_strPath = strurl;
          pItem->m_bIsFolder = true;
          if(((dwDisplayType == RESOURCEDISPLAYTYPE_SERVER) && m_strHost.empty()) ||
             ((dwDisplayType == RESOURCEDISPLAYTYPE_SHARE) && !m_strHost.empty()))
            items.Add(pItem);
        }

        // If the NETRESOURCE structure represents a container resource, 
        //  call the EnumerateFunc function recursively.
        if (RESOURCEUSAGE_CONTAINER == (lpnrLocal[i].dwUsage & RESOURCEUSAGE_CONTAINER))
          EnumerateFunc(&lpnrLocal[i], items);
      }
    }
    // Process errors.
    //
    else if (dwResultEnum != ERROR_NO_MORE_ITEMS) 
    {
      CLog::Log(LOGERROR,"WNetEnumResource failed with error %d\n", dwResultEnum);
      break;
    }
  }
  //
  // End do.
  //
  while (dwResultEnum != ERROR_NO_MORE_ITEMS);
  //
  // Call the GlobalFree function to free the memory.
  //
  GlobalFree((HGLOBAL) lpnrLocal);
  //
  // Call WNetCloseEnum to end the enumeration.
  //
  dwResult = WNetCloseEnum(hEnum);

  if (dwResult != NO_ERROR) 
  {
      //
      // Process errors.
      //
      CLog::Log(LOGERROR,"WNetCloseEnum failed with error %d\n", dwResult);
      return false;
  }

  return true;
}