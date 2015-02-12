/*
*      Copyright (C) 2014 Team XBMC
*      http://xbmc.org
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

#ifdef TARGET_WINDOWS

#include "Win32SMBDirectory.h"
#include "FileItem.h"
#include "win32/WIN32Util.h"
#include "utils/SystemInfo.h"
#include "utils/CharsetConverter.h"
#include "URL.h"
#include "utils/win32/Win32Log.h"
#include "PasswordManager.h"
#include "utils/auto_buffer.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif // WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <Winnetwk.h>
#pragma comment(lib, "mpr.lib")

#include <lm.h>
#pragma comment(lib, "Netapi32.lib")

#include <cassert>

using namespace XFILE;

// local helper
static inline bool worthTryToConnect(const DWORD lastErr)
{
  return lastErr != ERROR_FILE_NOT_FOUND      && lastErr != ERROR_BAD_NET_NAME  &&
         lastErr != ERROR_NO_NET_OR_BAD_PATH  && lastErr != ERROR_NO_NETWORK    &&
         lastErr != ERROR_BAD_NETPATH;
}

/**
* Get servers recursively or get shares from specific server
* @param basePathToScanPtr the pointer to start point for scan
* @param urlPrefixForItems path prefix to be putted into list items
* @param items             the list of the collected items
* @param getShares         set to true to get shares, set for false to get servers
* @return true if succeed, false otherwise
*/
static bool localGetNetworkResources(struct _NETRESOURCEW* basePathToScanPtr, const std::string& urlPrefixForItems, CFileItemList& items, bool getShares);
static bool localGetShares(const std::wstring& serverNameToScan, const std::string& urlPrefixForItems, CFileItemList& items);

// check for empty string, remove trailing slash if any, convert to win32 form
inline static std::wstring prepareWin32SMBDirectoryName(const CURL& url)
{
  assert(url.IsProtocol("smb"));

  if (url.GetHostName().empty() || url.GetShareName().empty())
    return std::wstring(); // can't use win32 standard file API, return empty string

  std::wstring nameW(CWIN32Util::ConvertPathToWin32Form("\\\\?\\UNC\\" + url.GetHostName() + '\\' + url.GetFileName()));
  if (!nameW.empty() && nameW.back() == L'\\')
    nameW.pop_back(); // remove slash at the end if any

  return nameW;
}

CWin32SMBDirectory::CWin32SMBDirectory(void)
{}

CWin32SMBDirectory::~CWin32SMBDirectory(void)
{}

bool CWin32SMBDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  assert(url.IsProtocol("smb"));
  items.Clear();

  if (url.GetShareName().empty())
  { // empty share name means that requested list of hosts or list of shares
    if (GetNetworkResources(url, items))
      return true;
    
    // try to connect and authenticate
    CURL authConnUrl(url);
    if (!ConnectAndAuthenticate(authConnUrl, (m_flags & DIR_FLAG_ALLOW_PROMPT) != 0))
      return false;
    items.Clear();
    return GetNetworkResources(authConnUrl, items);
  }

  if (url.GetHostName().empty())
    return false; // share name is set, but host name is empty

  /* Get file directory content by using standard win32 file API */
  std::wstring searchMask(CWIN32Util::ConvertPathToWin32Form("\\\\?\\UNC\\" + url.GetHostName() + '\\' + url.GetFileName()));
  if (searchMask.empty())
    return false;

  // TODO: support m_strFileMask, require rewrite of internal caching
  if (searchMask.back() == '\\')
    searchMask += L'*';
  else
    searchMask += L"\\*";

  HANDLE hSearch;
  WIN32_FIND_DATAW findData = {};
  CURL authUrl(url); // ConnectAndAuthenticate may update url with username and password

  if (g_sysinfo.IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin7))
    hSearch = FindFirstFileExW(searchMask.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
  else
    hSearch = FindFirstFileExW(searchMask.c_str(), FindExInfoStandard, &findData, FindExSearchNameMatch, NULL, 0);

  if (hSearch == INVALID_HANDLE_VALUE)
  {
    DWORD searchErr = GetLastError();
    if (!worthTryToConnect(searchErr))
      return false;

    if (ConnectAndAuthenticate(authUrl, (m_flags & DIR_FLAG_ALLOW_PROMPT) != 0))
    {
      if (g_sysinfo.IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin7))
        hSearch = FindFirstFileExW(searchMask.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
      else
        hSearch = FindFirstFileExW(searchMask.c_str(), FindExInfoStandard, &findData, FindExSearchNameMatch, NULL, 0);
      searchErr = GetLastError();
    }
    if (hSearch == INVALID_HANDLE_VALUE)
    {
      if (searchErr == ERROR_ACCESS_DENIED ||searchErr == ERROR_BAD_USERNAME ||
          searchErr == ERROR_INVALID_PASSWORD || searchErr == ERROR_LOGON_FAILURE)
      {
        if ((m_flags & DIR_FLAG_ALLOW_PROMPT) != 0)
          RequireAuthentication(authUrl);
        
        return false;
      }

      return (searchErr == ERROR_FILE_NOT_FOUND) ? RealExists(url, false) : false;
    }
  }

  std::string pathWithSlash(authUrl.Get());
  assert(!pathWithSlash.empty());
  if (pathWithSlash.back() != '/')
    pathWithSlash.push_back('/');

  do
  {
    std::wstring itemNameW(findData.cFileName);
    if (itemNameW == L"." || itemNameW == L".." || itemNameW.empty())
      continue;
    
    std::string itemName;
    if (!g_charsetConverter.wToUTF8(itemNameW, itemName, true) || itemName.empty())
    {
      CLog::LogF(LOGERROR, "Can't convert wide string item name to UTF-8");
      continue;
    }

    CFileItemPtr pItem(new CFileItem(itemName));

    pItem->m_bIsFolder = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
    if (pItem->m_bIsFolder)
      pItem->SetPath(pathWithSlash + itemName + '/');
    else
      pItem->SetPath(pathWithSlash + itemName);

    if ((findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0 
          || itemName.front() == '.') // mark files starting from dot as hidden
      pItem->SetProperty("file:hidden", true);

    // calculation of size and date costs a little on win32 
    // so DIR_FLAG_NO_FILE_INFO flag is ignored
    FILETIME localTime;
    if (FileTimeToLocalFileTime(&findData.ftLastWriteTime, &localTime) == TRUE)
      pItem->m_dateTime = localTime;
    else
      pItem->m_dateTime.SetValid(false);
 
    if (!pItem->m_bIsFolder)
        pItem->m_dwSize = (__int64(findData.nFileSizeHigh) << 32) + findData.nFileSizeLow;

    items.Add(pItem);
  } while (FindNextFileW(hSearch, &findData));

  FindClose(hSearch);
  
  return true;
}

bool CWin32SMBDirectory::Create(const CURL& url)
{
  return RealCreate(url, true);
}

bool CWin32SMBDirectory::RealCreate(const CURL& url, bool tryToConnect)
{
  assert(url.IsProtocol("smb"));
  if (url.GetHostName().empty() || url.GetShareName().empty() || url.GetFileName() == url.GetShareName())
    return false; // can't create new hosts or shares

  std::wstring nameW(prepareWin32SMBDirectoryName(url));
  if (nameW.empty())
    return false;

  if (!CreateDirectoryW(nameW.c_str(), NULL))
  {
    if (GetLastError() == ERROR_ALREADY_EXISTS)
      return RealExists(url, false); // is it file or directory?
    else
    {
      if (tryToConnect && worthTryToConnect(GetLastError()))
      {
        CURL authUrl(url);
        if (ConnectAndAuthenticate(authUrl))
          return RealCreate(authUrl, false);
      }
      return false;
    }
  }

  // if directory name starts from dot, make it hidden
  const size_t lastSlashPos = nameW.rfind(L'\\');
  if (lastSlashPos < nameW.length() - 1 && nameW[lastSlashPos + 1] == L'.')
  {
    DWORD dirAttrs = GetFileAttributesW(nameW.c_str());
    if (dirAttrs == INVALID_FILE_ATTRIBUTES || !SetFileAttributesW(nameW.c_str(), dirAttrs | FILE_ATTRIBUTE_HIDDEN))
      CLog::LogF(LOGWARNING, "Can't set hidden attribute for newly created directory \"%s\"", url.Get().c_str());
  }

  return true;
}

bool CWin32SMBDirectory::Exists(const CURL& url)
{
  return RealExists(url, true);
}

// this functions can check for: 
// * presence of directory on remove share (smb://server/share/dir)
// * presence of remote share on server (smb://server/share)
// * presence of smb server in network (smb://server)
bool CWin32SMBDirectory::RealExists(const CURL& url, bool tryToConnect)
{
  assert(url.IsProtocol("smb"));

  if (url.GetHostName().empty())
    return true; // 'root' of network is always exist
    
  // TODO: use real caseless string comparison everywhere in this function
  if (url.GetShareName().empty() || url.GetShareName() == url.GetFileName())
  {
    if (!url.GetShareName().empty())
    {
      std::wstring serverNameW;
      std::wstring shareNameW;
      SHARE_INFO_1* info = NULL;
      // try fast way
      if (g_charsetConverter.utf8ToW("\\\\" + url.GetHostName(), serverNameW, false, false, true) &&
          g_charsetConverter.utf8ToW(url.GetShareName(), shareNameW, false, false, true) &&
          NetShareGetInfo((LPWSTR)serverNameW.c_str(), (LPWSTR)shareNameW.c_str(), 1, (LPBYTE*)&info) == NERR_Success)
      {
        const bool ret = ((info->shi1_type & STYPE_MASK) == STYPE_DISKTREE);
        NetApiBufferFree(info);
        return ret;
      }
      // fallback to slow check
    }
    CFileItemList entries;
    CURL baseUrl(url);
    if (url.GetShareName().empty())
      baseUrl.SetHostName(""); // scan network for servers
    else
    {
      baseUrl.Reset(); // hack to reset ShareName
      baseUrl.SetProtocol("smb");
      baseUrl.SetHostName(url.GetHostName()); // scan server for shares
    }
    
    if (!GetNetworkResources(baseUrl, entries))
    {
      if (tryToConnect && !url.GetShareName().empty())
      {
        CURL authUrl(url);
        if (ConnectAndAuthenticate(authUrl))
          return RealExists(authUrl, false);
      }
      return false;
    }
    const std::string& searchStr = (url.GetShareName().empty()) ? url.GetHostName() : url.GetShareName();
    const VECFILEITEMS entrVec = entries.GetList();
    for (VECFILEITEMS::const_iterator it = entrVec.begin(); it != entrVec.end(); ++it)
    {
      if ((*it)->GetLabel() == searchStr)
        return true;
    }
    return false;
  }

  // use standard win32 file API
  std::wstring nameW(prepareWin32SMBDirectoryName(url));
  if (nameW.empty())
    return false;

  DWORD fileAttrs = GetFileAttributesW(nameW.c_str());
  if (fileAttrs != INVALID_FILE_ATTRIBUTES)
    return (fileAttrs & FILE_ATTRIBUTE_DIRECTORY) != 0; // is file or directory?

  if (tryToConnect && worthTryToConnect(GetLastError()))
  {
    CURL authUrl(url);
    if (ConnectAndAuthenticate(authUrl))
      return RealExists(authUrl, false);
  }

  return false;
}

bool CWin32SMBDirectory::Remove(const CURL& url)
{
  assert(url.IsProtocol("smb"));
  std::wstring nameW(prepareWin32SMBDirectoryName(url));
  if (nameW.empty())
    return false;

  if (RemoveDirectoryW(nameW.c_str()))
    return true;

  if (!worthTryToConnect(GetLastError()))
    return false;

  CURL authUrl(url);
  if (ConnectAndAuthenticate(authUrl) && RemoveDirectoryW(nameW.c_str()))
    return true;

  return !RealExists(url, false);
}

bool CWin32SMBDirectory::GetNetworkResources(const CURL& basePath, CFileItemList& items)
{
  assert(basePath.GetShareName().empty()); // this function returns only servers or shares

  std::string hostName(basePath.GetHostName());
  if (hostName.empty())
    return localGetNetworkResources(NULL, basePath.Get(), items, false); // get all servers from network
  
  // get all shares from server
  std::string basePathStr(basePath.Get());
  if (basePathStr.empty())
    return false;
  if (basePathStr.back() != '/')
    basePathStr.push_back('/');

  std::wstring remoteName;
  if (!basePathStr.empty() && !g_charsetConverter.utf8ToW("\\\\" + basePath.GetHostName(), remoteName, false, false, true))
  {
    CLog::LogF(LOGERROR, "can't convert host name \"%s\" to wide character form", basePath.GetHostName().c_str());
    return false;
  }

  _NETRESOURCEW netResBasePath = {};
  netResBasePath.dwScope = RESOURCE_GLOBALNET;
  netResBasePath.dwType = RESOURCETYPE_ANY;
  netResBasePath.dwDisplayType = RESOURCEDISPLAYTYPE_SERVER;
  netResBasePath.dwUsage = RESOURCEUSAGE_CONTAINER;
  netResBasePath.lpRemoteName = (LPWSTR)remoteName.c_str();

  return localGetNetworkResources(&netResBasePath, basePathStr, items, true);
}

/**
 * Get servers recursively or get shares from specific server
 * @param basePathToScanPtr the pointer to start point for scan
 * @param urlPrefixForItems path prefix to be putted into list items
 * @param items             the list of the collected items
 * @param getShares         set to true to get shares, set for false to get servers
 * @return true if succeed, false otherwise
 */
static bool localGetNetworkResources(struct _NETRESOURCEW* basePathToScanPtr, const std::string& urlPrefixForItems, CFileItemList& items, bool getShares)
{
  assert(!basePathToScanPtr || (basePathToScanPtr->dwUsage & RESOURCEUSAGE_CONTAINER));
  assert(!basePathToScanPtr || basePathToScanPtr->dwScope == RESOURCE_GLOBALNET);
  assert(!basePathToScanPtr || (basePathToScanPtr->lpRemoteName != NULL && basePathToScanPtr->lpRemoteName[0]));
  assert(!getShares || basePathToScanPtr); // can't get shares without host
  assert(urlPrefixForItems.compare(0, 6, "smb://", 6) == 0); // 'urlPrefixForItems' must be in form 'smb://[[user[:pass]@]ServerName]'
  assert(urlPrefixForItems.length() >= 6);
  assert(!getShares || urlPrefixForItems.back() == '/');

  if (basePathToScanPtr && basePathToScanPtr->dwDisplayType == RESOURCEDISPLAYTYPE_SERVER &&
      getShares)
  {
    if (localGetShares(basePathToScanPtr->lpRemoteName, urlPrefixForItems, items))
      return true;

    CLog::LogFW(LOGNOTICE, L"Can't read shares for \"%ls\" by localGetShares(), fallback to standard method", basePathToScanPtr->lpRemoteName);
  }

  HANDLE netEnum;
  DWORD result;
  result = WNetOpenEnumW(RESOURCE_GLOBALNET, getShares ? RESOURCETYPE_DISK : RESOURCETYPE_ANY, 0, basePathToScanPtr, &netEnum);
  if (result != NO_ERROR)
  {
    if (basePathToScanPtr)
    {
      std::wstring providerName;
      if (basePathToScanPtr->lpProvider && basePathToScanPtr->lpProvider[0] != 0)
        providerName.assign(L" (provider \"").append(basePathToScanPtr->lpProvider).append(L"\")");
      CLog::LogFW(LOGNOTICE, L"Can't open network enumeration for \"%ls\"%ls. Error: %lu", basePathToScanPtr->lpRemoteName, providerName.c_str(), (unsigned long)result);
    }
    else
      CLog::LogF(LOGERROR, "Can't open network enumeration for network root. Error: %lu", (unsigned long)result);

    return false;
  }

  XUTILS::auto_buffer buf(size_t(32 * 1024));
  bool errorFlag = false;
  do
  {
    DWORD resCount = -1;
    DWORD bufSize = buf.size();
    result = WNetEnumResourceW(netEnum, &resCount, buf.get(), &bufSize);
    if (result == NO_ERROR)
    { 
      if (bufSize > buf.size())
      { // buffer is too small
        buf.allocate(bufSize); // discard buffer content and extend the buffer
        bufSize = buf.size();
        result = WNetEnumResourceW(netEnum, &resCount, buf.get(), &bufSize);
        if (result != NO_ERROR || bufSize > buf.size())
          errorFlag = true; // hardly ever happens
      }

      for (unsigned int i = 0; i < resCount && !errorFlag; i++)
      {
        _NETRESOURCEW& curResource = ((_NETRESOURCEW*)buf.get())[i];
        
        /* check and collect servers */
        if (!getShares && curResource.dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)
        {
          if (curResource.lpRemoteName != NULL)
          {
            std::wstring remoteName(curResource.lpRemoteName);
            if (remoteName.length() > 2 && remoteName.compare(0, 2, L"\\\\", 2) == 0)
            {
              std::string remoteNameUtf8;
              if (g_charsetConverter.wToUTF8(remoteName.substr(2), remoteNameUtf8, true) && !remoteNameUtf8.empty())
              {
                CFileItemPtr pItem(new CFileItem(remoteNameUtf8));
                pItem->SetPath(urlPrefixForItems + remoteNameUtf8 + '/');
                pItem->m_bIsFolder = true;
                items.Add(pItem);
              }
              else
                CLog::LogFW(LOGERROR, L"Can't convert server wide string name \"%ls\" to UTF-8 encoding", remoteName.substr(2).c_str());
            }
            else
              CLog::LogFW(LOGERROR, L"Skipping server name \"%ls\" without '\\' prefix", remoteName.c_str());
          }
          else
            CLog::LogF(LOGERROR, "Skipping server with empty remote name");
        }
        
        /* check and collect shares */
        if (getShares && (curResource.dwDisplayType == RESOURCEDISPLAYTYPE_SHARE ||
                          curResource.dwDisplayType == RESOURCEDISPLAYTYPE_SHAREADMIN) &&
                          curResource.dwType != RESOURCETYPE_PRINT)
        {
          if (curResource.lpRemoteName != NULL)
          {
            std::wstring serverShareName(curResource.lpRemoteName);
            if (serverShareName.length() > 2 && serverShareName.compare(0, 2, L"\\\\", 2) == 0)
            {
              const size_t slashPos = serverShareName.rfind('\\');
              if (slashPos < serverShareName.length() - 1) // slash must be not on last position
              {
                std::string shareNameUtf8;
                if (g_charsetConverter.wToUTF8(serverShareName.substr(slashPos + 1), shareNameUtf8, true) && !shareNameUtf8.empty())
                {
                  CFileItemPtr pItem(new CFileItem(shareNameUtf8));
                  pItem->SetPath(urlPrefixForItems + shareNameUtf8 + '/');
                  pItem->m_bIsFolder = true;
                  if (curResource.dwDisplayType == RESOURCEDISPLAYTYPE_SHAREADMIN)
                    pItem->SetProperty("file:hidden", true);

                  items.Add(pItem);
                }
                else
                  CLog::LogFW(LOGERROR, L"Can't convert server and share wide string name \"%ls\" to UTF-8 encoding", serverShareName.substr(slashPos + 1).c_str());
              }
              else
                CLog::LogFW(LOGERROR, L"Can't find name of share in remote name \"%ls\"", serverShareName.c_str());
            }
            else
              CLog::LogFW(LOGERROR, L"Skipping name \"%ls\" without '\\' prefix", serverShareName.c_str());
          }
          else
            CLog::LogF(LOGERROR, "Skipping share with empty remote name");
        }

        /* recursively collect servers from container */
        if (!getShares && (curResource.dwUsage & RESOURCEUSAGE_CONTAINER) &&
            curResource.dwDisplayType != RESOURCEDISPLAYTYPE_SERVER) // don't scan servers for other servers
        {
          if (curResource.lpRemoteName != NULL && curResource.lpRemoteName[0] != 0)
          {
            if (!localGetNetworkResources(&curResource, urlPrefixForItems, items, false))
              CLog::LogFW(LOGNOTICE, L"Can't get servers from \"%ls\", skipping", curResource.lpRemoteName);
          }
          else
            CLog::Log(LOGERROR, "%s: Skipping container with empty remote name", __FUNCTION__);
        }
      }
    }
  } while (result == NO_ERROR && !errorFlag);

  WNetCloseEnum(netEnum);

  if (errorFlag || result != ERROR_NO_MORE_ITEMS)
  {
    if (basePathToScanPtr && basePathToScanPtr->lpRemoteName)
    {
      if (errorFlag)
        CLog::LogFW(LOGERROR, L"Error loading content for \"%ls\"", basePathToScanPtr->lpRemoteName);
      else
        CLog::LogFW(LOGERROR, L"Error (%lu) loading content for \"%ls\"", (unsigned long)result, basePathToScanPtr->lpRemoteName);
    }
    else
    {
      if (errorFlag)
        CLog::LogF(LOGERROR, "Error loading content of network root");
      else
        CLog::LogF(LOGERROR, "Error (%lu) loading content of network root", (unsigned long)result);
    }
    return false;
  }

  return true;
}

// advanced shares enumeration function
// if this function is failed, simple 'localGetNetworkResources()' can be used
static bool localGetShares(const std::wstring& serverNameToScan, const std::string& urlPrefixForItems, CFileItemList& items)
{
  assert(serverNameToScan.compare(0, 2, L"\\\\", 2) == 0); // 'serverNameToScan' must be in form '\\ServerName'
  assert(serverNameToScan.length() > 2);
  assert(urlPrefixForItems.compare(0, 6, "smb://", 6) == 0); // 'urlPrefixForItems' must be in form 'smb://[user[:pass]@]ServerName/'
  assert(urlPrefixForItems.length() > 7);
  assert(urlPrefixForItems.back() == '/');

  CFileItemList locItems; // store items locally until last one is successfully loaded
  
  NET_API_STATUS enumResult;
  DWORD hEnumResume = 0;
  bool errorFlag = false;
  do
  {
    SHARE_INFO_1* shareInfos = NULL;
    DWORD count, totalCount;
    enumResult = NetShareEnum((LPWSTR)serverNameToScan.c_str(), 1, (LPBYTE*)&shareInfos, MAX_PREFERRED_LENGTH, &count, &totalCount, &hEnumResume);
    if (enumResult == NERR_Success || enumResult == ERROR_MORE_DATA)
    {
      for (unsigned int i = 0; i < count && !errorFlag; i++)
      {
        SHARE_INFO_1& curShare = shareInfos[i];
        if ((curShare.shi1_type & STYPE_MASK) == STYPE_DISKTREE)
        {
          std::string shareNameUtf8;
          if (curShare.shi1_netname && curShare.shi1_netname[0] &&
              g_charsetConverter.wToUTF8(curShare.shi1_netname, shareNameUtf8, true) && !shareNameUtf8.empty())
          {
            CFileItemPtr pItem(new CFileItem(shareNameUtf8));
            pItem->SetPath(urlPrefixForItems + shareNameUtf8 + '/');
            pItem->m_bIsFolder = true;
            if ((curShare.shi1_type & STYPE_SPECIAL) != 0 || shareNameUtf8.back() == '$')
              pItem->SetProperty("file:hidden", true);

            items.Add(pItem);
          }
          else
            errorFlag = true;
        }
      }
      NetApiBufferFree(shareInfos);
    }
  } while (!errorFlag && enumResult == ERROR_MORE_DATA);

  if (enumResult != NERR_Success || errorFlag)
    return false; // do not touch 'items' with partial result, allow fallback to another shares enumeration method

  items.Append(locItems); // all shares loaded, store result
  return true;
}

bool CWin32SMBDirectory::ConnectAndAuthenticate(CURL& url, bool allowPromptForCredential /*= false*/)
{
  assert(url.IsProtocol("smb"));
  if (url.GetHostName().empty())
    return false; // can't connect to empty host name
  
  if (url.GetUserName().empty() && url.GetPassWord().empty())
    CPasswordManager::GetInstance().AuthenticateURL(url); // set username and password if any

  /* convert everything to wide strings */
  std::wstring serverNameW;
  if (!g_charsetConverter.utf8ToW(url.GetHostName(), serverNameW, false, false, true))
  {
    CLog::LogF(LOGERROR, "Can't convert server name \"%s\" to wide string", url.GetHostName().c_str());
    return false;
  }
  serverNameW = L"\\\\" + serverNameW;

  std::string serverShareName; // for error descriptions
  std::wstring serverShareNameW;
  if (!url.GetShareName().empty())
  {
    serverShareName = "\\\\" + url.GetHostName() + "\\" + url.GetShareName();
    if (!g_charsetConverter.utf8ToW(serverShareName, serverShareNameW, false, false, true))
    {
      CLog::LogF(LOGERROR, "Can't convert share name \"%s\" to wide string", serverShareName.c_str());
      return false;
    }
  }
  else
  {
    serverShareName = "\\\\" + url.GetHostName();
    serverShareNameW = serverNameW;
  }

  std::wstring usernameW;
  if (!url.GetUserName().empty() && !g_charsetConverter.utf8ToW(url.GetUserName(), usernameW, false, false, true))
  {
    CLog::LogF(LOGERROR, "Can't convert username \"%s\" to wide string", url.GetUserName().c_str());
    return false;
    std::wstring domainW;
    if (!url.GetDomain().empty() && !g_charsetConverter.utf8ToW(url.GetDomain(), domainW, false, false, true))
    {
      CLog::LogF(LOGERROR, "Can't convert domain name \"%s\" to wide string", url.GetDomain().c_str());
      return false;
    }
    if (!domainW.empty())
      usernameW += L'@' + domainW;
  }

  std::wstring passwordW;
  if (!url.GetPassWord().empty() && !g_charsetConverter.utf8ToW(url.GetPassWord(), passwordW, false, false, true))
  {
    CLog::LogF(LOGERROR, "Can't convert password to wide string");
    return false;
  }

  std::string loginDescr;
  if (url.GetUserName().empty())
    loginDescr = "without username";
  else
    loginDescr = "with username \"" + url.GetUserName() + (url.GetDomain().empty() ? "" : "@" + url.GetDomain()) + "\"";

  loginDescr += url.GetPassWord().empty() ? " and without password" : " and with password";
  NETRESOURCEW connInfo = {};
  connInfo.dwType = RESOURCETYPE_ANY;
  connInfo.lpRemoteName = (LPWSTR)serverShareNameW.c_str();
  DWORD connRes;
  for (int i = 0; i < 3; i++) // make up to three attempts to connect
  {
    connRes = WNetAddConnection2W(&connInfo, passwordW.empty() ? NULL : (LPWSTR)passwordW.c_str(),
                                  usernameW.empty() ? NULL : (LPWSTR)usernameW.c_str(), CONNECT_TEMPORARY);
    if (connRes == NO_ERROR)
    {
      CLog::LogF(LOGDEBUG, "Connected to \"%s\" %s", serverShareName.c_str(), loginDescr.c_str());
      return true;
    }
    
    if (connRes == ERROR_ACCESS_DENIED || connRes == ERROR_BAD_USERNAME || connRes == ERROR_INVALID_PASSWORD ||
        connRes == ERROR_LOGON_FAILURE || connRes == ERROR_LOGON_TYPE_NOT_GRANTED || connRes == ERROR_LOGON_NOT_GRANTED)
    {
      if (connRes == ERROR_ACCESS_DENIED)
        CLog::LogF(LOGERROR, "Doesn't have permissions to access \"%s\" %s", serverShareName.c_str(), loginDescr.c_str());
      else
        CLog::LogF(LOGERROR, "Username/password combination was not accepted by \"%s\" when trying to connect %s", serverShareName.c_str(), loginDescr.c_str());
      if (allowPromptForCredential)
        RequireAuthentication(url);

      return false; // don't try any more
    }
    else if (connRes == ERROR_BAD_NET_NAME || connRes == ERROR_NO_NET_OR_BAD_PATH || connRes == ERROR_NO_NETWORK)
    {
      CLog::LogF(LOGERROR, "Can't find \"%s\"", serverShareName.c_str());
      return false; // don't try any more
    }
    else if (connRes == ERROR_BUSY)
      CLog::LogF(LOGNOTICE, "Network is busy for \"%s\"", serverShareName.c_str());
    else if (connRes == ERROR_SESSION_CREDENTIAL_CONFLICT)
    {
      CLog::LogF(LOGWARNING, "Can't connect to \"%s\" %s because of conflict of credential. Will try to close current connections.", serverShareName.c_str(), loginDescr.c_str());
      WNetCancelConnection2W((LPWSTR)serverShareNameW.c_str(), 0, FALSE);
      WNetCancelConnection2W((LPWSTR)(serverNameW + L"\\IPC$").c_str(), 0, FALSE);
      WNetCancelConnection2W((LPWSTR)serverNameW.c_str(), 0, FALSE);
    }
  }

  CLog::LogF(LOGWARNING, "Can't connect to \"%s\" %s. Error code: %lu", serverShareName.c_str(), loginDescr.c_str(), (unsigned long)connRes);
  return false;
}

#endif // TARGET_WINDOWS
