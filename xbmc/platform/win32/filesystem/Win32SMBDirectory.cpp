/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Win32SMBDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "PasswordManager.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "utils/CharsetConverter.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"
#include "platform/win32/WIN32Util.h"
#include "platform/win32/network/WSDiscoveryWin32.h"

#include <Windows.h>
#include <Winnetwk.h>
#pragma comment(lib, "mpr.lib")

#include <lm.h>
#pragma comment(lib, "Netapi32.lib")

#include <cassert>

using namespace XFILE;

using KODI::PLATFORM::WINDOWS::FromW;

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
static bool localGetServers(const std::string& urlPrefixForItems, CFileItemList& items);

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

  //! @todo support m_strFileMask, require rewrite of internal caching
  if (searchMask.back() == '\\')
    searchMask += L'*';
  else
    searchMask += L"\\*";

  HANDLE hSearch;
  WIN32_FIND_DATAW findData = {};
  CURL authUrl(url); // ConnectAndAuthenticate may update url with username and password

  hSearch = FindFirstFileExW(searchMask.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);

  if (hSearch == INVALID_HANDLE_VALUE)
  {
    DWORD searchErr = GetLastError();
    if (!worthTryToConnect(searchErr))
      return false;

    if (ConnectAndAuthenticate(authUrl, (m_flags & DIR_FLAG_ALLOW_PROMPT) != 0))
    {
      hSearch = FindFirstFileExW(searchMask.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
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
    KODI::TIME::FileTime fileTime;
    fileTime.lowDateTime = findData.ftLastWriteTime.dwLowDateTime;
    fileTime.highDateTime = findData.ftLastWriteTime.dwHighDateTime;
    KODI::TIME::FileTime localTime;
    if (KODI::TIME::FileTimeToLocalFileTime(&fileTime, &localTime) == TRUE)
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
      CLog::LogF(LOGWARNING, "Can't set hidden attribute for newly created directory \"{}\"",
                 url.Get());
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

  //! @todo use real caseless string comparison everywhere in this function
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
    for (const auto& it : entrVec)
    {
      if (it->GetLabel() == searchStr)
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
    CLog::LogF(LOGERROR, "can't convert host name \"{}\" to wide character form",
               basePath.GetHostName());
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

    CLog::LogF(LOGWARNING,
               "Can't read shares for \"{}\" by localGetShares(), fallback to old method",
               FromW(basePathToScanPtr->lpRemoteName));
  }

  // Get servers using WS-Discovery protocol
  if (!getShares)
  {
    if (localGetServers(urlPrefixForItems, items))
      return true;

    CLog::LogF(LOGWARNING, "Can't locate servers by localGetServers(), fallback to old method");
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
      CLog::LogF(LOGINFO, "Can't open network enumeration for \"{}\"{}. Error: {}",
                 FromW(basePathToScanPtr->lpRemoteName), FromW(providerName),
                 static_cast<unsigned long>(result));
    }
    else
      CLog::LogF(LOGERROR, "Can't open network enumeration for network root. Error: {}",
                 static_cast<unsigned long>(result));

    return false;
  }

  std::vector<char> buf(size_t(32 * 1024));
  bool errorFlag = false;
  do
  {
    DWORD resCount = -1;
    size_t bufSize = buf.size();
    result = WNetEnumResourceW(netEnum, &resCount, buf.data(), reinterpret_cast<LPDWORD>(&bufSize));
    if (result == NO_ERROR)
    {
      if (bufSize > buf.size())
      { // buffer is too small
        buf.resize(bufSize); // discard buffer content and extend the buffer
        bufSize = buf.size();
        result =
            WNetEnumResourceW(netEnum, &resCount, buf.data(), reinterpret_cast<LPDWORD>(&bufSize));
        if (result != NO_ERROR || bufSize > buf.size())
          errorFlag = true; // hardly ever happens
      }

      for (unsigned int i = 0; i < resCount && !errorFlag; i++)
      {
        _NETRESOURCEW& curResource = ((_NETRESOURCEW*)buf.data())[i];

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
                CLog::LogF(LOGERROR,
                           "Can't convert server wide string name \"{}\" to UTF-8 encoding",
                           FromW(remoteName.substr(2)));
            }
            else
              CLog::LogF(LOGERROR, "Skipping server name \"{}\" without '\\' prefix",
                         FromW(remoteName));
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
                {
                  CLog::LogF(
                      LOGERROR,
                      "Can't convert server and share wide string name \"{}\" to UTF-8 encoding",
                      FromW(serverShareName.substr(slashPos + 1)));
                }
              }
              else
              {
                CLog::LogF(LOGERROR, "Can't find name of share in remote name \"{}\"",
                           FromW(serverShareName));
              }
            }
            else
            {
              CLog::LogF(LOGERROR, "Skipping name \"{}\" without '\\' prefix",
                         FromW(serverShareName));
            }
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
            {
              CLog::LogF(LOGINFO, "Can't get servers from \"{}\", skipping",
                         FromW(curResource.lpRemoteName));
            }
          }
          else
            CLog::Log(LOGERROR, "{}: Skipping container with empty remote name", __FUNCTION__);
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
      {
        CLog::LogF(LOGERROR, "Error loading content for \"{}\"",
                   FromW(basePathToScanPtr->lpRemoteName));
      }
      else
      {
        CLog::LogF(LOGERROR, "Error ({}) loading content for \"{}\"",
                   static_cast<unsigned long>(result), FromW(basePathToScanPtr->lpRemoteName));
      }
    }
    else
    {
      if (errorFlag)
        CLog::LogF(LOGERROR, "Error loading content of network root");
      else
        CLog::LogF(LOGERROR, "Error ({}) loading content of network root", (unsigned long)result);
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

// Get servers using WS-Discovery protocol
static bool localGetServers(const std::string& urlPrefixForItems, CFileItemList& items)
{
  WSDiscovery::CWSDiscoveryWindows& wsd =
      dynamic_cast<WSDiscovery::CWSDiscoveryWindows&>(CServiceBroker::GetWSDiscovery());

  // Get servers immediately from WSD daemon process
  if (wsd.IsRunning() && wsd.ThereAreServers())
  {
    for (const auto& ip : wsd.GetServersIPs())
    {
      std::wstring hostname = wsd.ResolveHostName(ip);
      std::string shareNameUtf8;
      if (g_charsetConverter.wToUTF8(hostname, shareNameUtf8, true) && !shareNameUtf8.empty())
      {
        CFileItemPtr pItem = std::make_shared<CFileItem>(shareNameUtf8);
        pItem->SetPath(urlPrefixForItems + shareNameUtf8 + '/');
        pItem->m_bIsFolder = true;
        items.Add(pItem);
      }
    }
    return true;
  }

  return false;
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
    CLog::LogF(LOGERROR, "Can't convert server name \"{}\" to wide string", url.GetHostName());
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
      CLog::LogF(LOGERROR, "Can't convert share name \"{}\" to wide string", serverShareName);
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
    CLog::LogF(LOGERROR, "Can't convert username \"{}\" to wide string", url.GetUserName());
    return false;
  }
  std::wstring domainW;
  if (!url.GetDomain().empty() && !g_charsetConverter.utf8ToW(url.GetDomain(), domainW, false, false, true))
  {
    CLog::LogF(LOGERROR, "Can't convert domain name \"{}\" to wide string", url.GetDomain());
    return false;
  }
  if (!domainW.empty())
    usernameW += L'@' + domainW;

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
    // try with provided user/password or NULL user/password (NULL = current Windows user/password)
    connRes = WNetAddConnection2W(&connInfo, passwordW.empty() ? NULL : (LPWSTR)passwordW.c_str(),
                                  usernameW.empty() ? NULL : (LPWSTR)usernameW.c_str(), CONNECT_TEMPORARY);

    // try with passwordless access (guest user and no password)
    if (usernameW.empty() && passwordW.empty() &&
        (connRes == ERROR_ACCESS_DENIED || connRes == ERROR_BAD_USERNAME ||
         connRes == ERROR_INVALID_PASSWORD || connRes == ERROR_LOGON_FAILURE ||
         connRes == ERROR_LOGON_TYPE_NOT_GRANTED || connRes == ERROR_LOGON_NOT_GRANTED))
      connRes = WNetAddConnection2W(&connInfo, L"", L"guest", CONNECT_TEMPORARY);

    if (connRes == NO_ERROR)
    {
      CLog::LogF(LOGDEBUG, "Connected to \"{}\" {}", serverShareName, loginDescr);
      return true;
    }

    if (connRes == ERROR_ACCESS_DENIED || connRes == ERROR_BAD_USERNAME || connRes == ERROR_INVALID_PASSWORD ||
        connRes == ERROR_LOGON_FAILURE || connRes == ERROR_LOGON_TYPE_NOT_GRANTED || connRes == ERROR_LOGON_NOT_GRANTED)
    {
      if (connRes == ERROR_ACCESS_DENIED)
        CLog::LogF(LOGERROR, "Doesn't have permissions to access \"{}\" {}", serverShareName,
                   loginDescr);
      else
        CLog::LogF(
            LOGERROR,
            "Username/password combination was not accepted by \"{}\" when trying to connect {}",
            serverShareName, loginDescr);
      if (allowPromptForCredential)
        RequireAuthentication(url);

      return false; // don't try any more
    }
    else if (connRes == ERROR_BAD_NET_NAME || connRes == ERROR_NO_NET_OR_BAD_PATH || connRes == ERROR_NO_NETWORK)
    {
      CLog::LogF(LOGERROR, "Can't find \"{}\"", serverShareName);
      return false; // don't try any more
    }
    else if (connRes == ERROR_BUSY)
      CLog::LogF(LOGINFO, "Network is busy for \"{}\"", serverShareName);
    else if (connRes == ERROR_SESSION_CREDENTIAL_CONFLICT)
    {
      CLog::LogF(LOGWARNING,
                 "Can't connect to \"{}\" {} because of conflict of credential. Will try to close "
                 "current connections.",
                 serverShareName, loginDescr);
      WNetCancelConnection2W((LPWSTR)serverShareNameW.c_str(), 0, FALSE);
      WNetCancelConnection2W((LPWSTR)(serverNameW + L"\\IPC$").c_str(), 0, FALSE);
      WNetCancelConnection2W((LPWSTR)serverNameW.c_str(), 0, FALSE);
    }
  }

  CLog::LogF(LOGWARNING, "Can't connect to \"{}\" {}. Error code: {}", serverShareName, loginDescr,
             (unsigned long)connRes);
  return false;
}
