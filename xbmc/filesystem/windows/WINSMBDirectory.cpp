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


#include "WINSMBDirectory.h"
#include "URL.h"
#include "utils/URIUtils.h"
#include "settings/GUISettings.h"
#include "FileItem.h"
#include "WIN32Util.h"
#include "utils/AutoPtrHandle.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "PasswordManager.h"
#include "Util.h"

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD) -1)
#endif


using namespace AUTOPTR;
using namespace XFILE;

CWINSMBDirectory::CWINSMBDirectory(void)
{
  m_bHost=false;
}

CWINSMBDirectory::~CWINSMBDirectory(void)
{
}

CStdString CWINSMBDirectory::GetLocal(const CStdString& strPath)
{
  CURL url(strPath);
  CStdString path( url.GetFileName() );
  if( url.GetProtocol().Equals("smb", false) )
  {
    CStdString host( url.GetHostName() );

    if(host.size() > 0)
    {
      path = "//" + host + "/" + path;
    }
  }
  path.Replace('/', '\\');
  return path;
}

bool CWINSMBDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{
  WIN32_FIND_DATAW wfd;

  CStdString strPath=strPath1;

  CURL url(strPath);

  if(url.GetShareName().empty())
  {
    LPNETRESOURCEW lpnr = NULL;
    bool ret;
    if(!url.GetHostName().empty())
    {
      lpnr = (LPNETRESOURCEW) GlobalAlloc(GPTR, 16384);
      if(lpnr == NULL)
        return false;

      ConnectToShare(url);
      CStdString strHost = "\\\\" + url.GetHostName();
      CStdStringW strHostW;
      g_charsetConverter.utf8ToW(strHost,strHostW);
      lpnr->lpRemoteName = (LPWSTR)strHostW.c_str();
      m_bHost = true;
      ret = EnumerateFunc(lpnr, items);
      GlobalFree((HGLOBAL) lpnr);
      m_bHost = false;
    }
    else
      ret = EnumerateFunc(lpnr, items);

    return ret;
  }

  memset(&wfd, 0, sizeof(wfd));
  //rebuild the URL
  CStdString strUNCShare = "\\\\" + url.GetHostName() + "\\" + url.GetFileName();
  strUNCShare.Replace("/", "\\");
  if(!URIUtils::HasSlashAtEnd(strUNCShare))
    strUNCShare.append("\\");

  CStdStringW strSearchMask;
  g_charsetConverter.utf8ToW(strUNCShare, strSearchMask, false);
  strSearchMask += "*";

  FILETIME localTime;
  CAutoPtrFind hFind ( FindFirstFileW(strSearchMask.c_str(), &wfd));

  // on error, check if path exists at all, this will return true if empty folder
  if (!hFind.isValid())
  {
    DWORD ret = GetLastError();
    if(ret == ERROR_INVALID_PASSWORD || ret == ERROR_LOGON_FAILURE || ret == ERROR_ACCESS_DENIED || ret == ERROR_INVALID_HANDLE)
    {
      if(ConnectToShare(url) == false)
        return false;
      hFind.attach(FindFirstFileW(strSearchMask.c_str(), &wfd));
    }
    else
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
            CStdString path = URIUtils::AddFileToFolder(strPath, strLabel);
            URIUtils::AddSlashAtEnd(path);
            pItem->SetPath(path);
            pItem->m_bIsFolder = true;
            FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
            pItem->m_dateTime=localTime;

            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
              pItem->SetProperty("file:hidden", true);

            items.Add(pItem);
          }
        }
        else
        {
          CFileItemPtr pItem(new CFileItem(strLabel));
          pItem->SetPath(URIUtils::AddFileToFolder(strPath, strLabel));
          pItem->m_bIsFolder = false;
          pItem->m_dwSize = CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          pItem->m_dateTime=localTime;

          if (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
            pItem->SetProperty("file:hidden", true);
          items.Add(pItem);
        }
      }
    }
    while (FindNextFileW((HANDLE)hFind, &wfd));
  }
  return true;
}

bool CWINSMBDirectory::Create(const char* strPath)
{
  CStdString strPath1 = GetLocal(strPath);
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
  CStdString strPath1 = GetLocal(strPath);
  g_charsetConverter.utf8ToW(strPath1, strWPath, false);
  return ::RemoveDirectoryW(strWPath) ? true : false;
}

bool CWINSMBDirectory::Exists(const char* strPath)
{
  CStdString strReplaced=GetLocal(strPath);
  CStdStringW strWReplaced;
  g_charsetConverter.utf8ToW(strReplaced, strWReplaced, false);
  // this will fail on shares, needs a subdirectory inside a share
  DWORD attributes = GetFileAttributesW(strWReplaced);
  if(attributes == INVALID_FILE_ATTRIBUTES)
    return false;
  if (FILE_ATTRIBUTE_DIRECTORY & attributes)
    return true;
  return false;
}

bool CWINSMBDirectory::EnumerateFunc(LPNETRESOURCEW lpnr, CFileItemList &items)
{
  DWORD dwResult, dwResultEnum;
  HANDLE hEnum;
  DWORD cbBuffer = 16384;     // 16K is a good size
  LPNETRESOURCEW lpnrLocal;   // pointer to enumerated structures
  DWORD cEntries = -1;        // enumerate all possible entries
  //
  // Call the WNetOpenEnum function to begin the enumeration.
  //
  dwResult = WNetOpenEnumW( RESOURCE_GLOBALNET,  // all network resources
                            RESOURCETYPE_DISK,   // all disk resources
                            0,                   // enumerate all resources
                            lpnr,                // NULL first time the function is called
                            &hEnum);             // handle to the resource

  if (dwResult != NO_ERROR)
  {
    CLog::Log(LOGERROR,"WnetOpenEnum failed with error %d", dwResult);
    if(dwResult == ERROR_EXTENDED_ERROR)
    {
      DWORD dwWNetResult, dwLastError;
      CHAR szDescription[256];
      CHAR szProvider[256];
      dwWNetResult = WNetGetLastError(&dwLastError, // error code
                            (LPSTR) szDescription,  // buffer for error description
                            sizeof(szDescription),  // size of error buffer
                            (LPSTR) szProvider,     // buffer for provider name
                            sizeof(szProvider));    // size of name buffer
      if(dwWNetResult == NO_ERROR)
        CLog::Log(LOGERROR,"%s failed with code %ld; %s", szProvider, dwLastError, szDescription);
    }
    return false;
  }
  //
  // Call the GlobalAlloc function to allocate resources.
  //
  lpnrLocal = (LPNETRESOURCEW) GlobalAlloc(GPTR, cbBuffer);
  if (lpnrLocal == NULL)
  {
    CLog::Log(LOGERROR,"Can't allocate buffer %d", cbBuffer);
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
    dwResultEnum = WNetEnumResourceW( hEnum,          // resource handle
                                      &cEntries,      // defined locally as -1
                                      lpnrLocal,      // LPNETRESOURCE
                                      &cbBuffer);     // buffer size
    //
    // If the call succeeds, loop through the structures.
    //
    if (dwResultEnum == NO_ERROR)
    {
      for (DWORD i = 0; i < cEntries; i++)
      {
        DWORD dwDisplayType = lpnrLocal[i].dwDisplayType;
        DWORD dwType = lpnrLocal[i].dwType;

        if((((dwDisplayType == RESOURCEDISPLAYTYPE_SERVER) && (m_bHost == false)) ||
           ((dwDisplayType == RESOURCEDISPLAYTYPE_SHARE) && m_bHost)) &&
           (dwType != RESOURCETYPE_PRINT))
        {
          CStdString strurl = "smb:";
          CStdStringW strRemoteNameW = lpnrLocal[i].lpRemoteName;
          CStdString  strName,strRemoteName;

          g_charsetConverter.wToUTF8(strRemoteNameW,strRemoteName);
          CLog::Log(LOGDEBUG,"Found Server/Share: %s", strRemoteName.c_str());

          strurl.append(strRemoteName);
          strurl.Replace("\\","/");
          CURL rooturl(strurl);
          rooturl.SetFileName("");

          if(!rooturl.GetShareName().empty())
            strName = rooturl.GetShareName();
          else
            strName = rooturl.GetHostName();

          strName.Replace("\\","");

          URIUtils::AddSlashAtEnd(strurl);
          CFileItemPtr pItem(new CFileItem(strName));
          pItem->SetPath(strurl);
          pItem->m_bIsFolder = true;
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
      CLog::Log(LOGERROR,"WNetEnumResource failed with error %d", dwResultEnum);
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
      CLog::Log(LOGERROR,"WNetCloseEnum failed with error %d", dwResult);
      return false;
  }

  return true;
}

bool CWINSMBDirectory::ConnectToShare(const CURL& url)
{
  NETRESOURCE nr;
  CURL urlIn(url);
  DWORD dwRet=-1;
  CStdString strUNC("\\\\"+url.GetHostName());
  if(!url.GetShareName().empty())
    strUNC.append("\\"+url.GetShareName());

  CStdString strPath;
  memset(&nr,0,sizeof(nr));
  nr.dwType = RESOURCETYPE_ANY;
  nr.lpRemoteName = (char*)strUNC.c_str();

  // in general we shouldn't need the password manager as we won't disconnect from shares yet
  CPasswordManager::GetInstance().AuthenticateURL(urlIn);

  CStdString strAuth = URLEncode(urlIn);

  while(dwRet != NO_ERROR)
  {
    strPath = URLEncode(urlIn);
    LPCTSTR pUser = urlIn.GetUserNameA().empty() ? NULL : (LPCTSTR)urlIn.GetUserNameA().c_str();
    LPCTSTR pPass = urlIn.GetPassWord().empty() ? NULL : (LPCTSTR)urlIn.GetPassWord().c_str();
    dwRet = WNetAddConnection2(&nr, pPass, pUser, CONNECT_TEMPORARY);
#ifdef _DEBUG
    CLog::Log(LOGDEBUG,"Trying to connect to %s with username(%s) and password(%s)", strUNC.c_str(), urlIn.GetUserNameA().c_str(), urlIn.GetPassWord().c_str());
#else
    CLog::Log(LOGDEBUG,"Trying to connect to %s with username(%s) and password(%s)", strUNC.c_str(), urlIn.GetUserNameA().c_str(), "XXXX");
#endif
    if(dwRet == ERROR_ACCESS_DENIED || dwRet == ERROR_INVALID_PASSWORD || dwRet == ERROR_LOGON_FAILURE)
    {
      CLog::Log(LOGERROR,"Couldn't connect to %s, access denied", strUNC.c_str());
      if (m_flags & DIR_FLAG_ALLOW_PROMPT)
        RequireAuthentication(urlIn.Get());
      break;
    }
    else if(dwRet == ERROR_SESSION_CREDENTIAL_CONFLICT)
    {
      DWORD dwRet2=-1;
      CStdString strRN = nr.lpRemoteName;
      do
      {
        dwRet2 = WNetCancelConnection2((LPCSTR)strRN.c_str(), 0, false);
        strRN.erase(strRN.find_last_of("\\"),CStdString::npos);
      }
      while(dwRet2 == ERROR_NOT_CONNECTED && !strRN.Equals("\\\\"));
    }
    else if(dwRet != NO_ERROR)
    {
      break;
    }
  }

  if(dwRet != NO_ERROR)
  {
    CLog::Log(LOGERROR,"Couldn't connect to %s, error code %d", strUNC.c_str(), dwRet);
    return false;
  }
  return true;
}

CStdString CWINSMBDirectory::URLEncode(const CURL &url)
{
  /* due to smb wanting encoded urls we have to build it manually */

  CStdString flat = "smb://";

  /* samba messes up of password is set but no username is set. don't know why yet */
  /* probably the url parser that goes crazy */
  if(url.GetUserName().length() > 0 /* || url.GetPassWord().length() > 0 */)
  {
    flat += url.GetUserName();
    flat += ":";
    flat += url.GetPassWord();
    flat += "@";
  }
  flat += url.GetHostName();

  /* okey sadly since a slash is an invalid name we have to tokenize */
  std::vector<CStdString> parts;
  std::vector<CStdString>::iterator it;
  CUtil::Tokenize(url.GetFileName(), parts, "/");
  for( it = parts.begin(); it != parts.end(); it++ )
  {
    flat += "/";
    flat += (*it);
  }

  /* okey options should go here, thou current samba doesn't support any */

  return flat;
}
