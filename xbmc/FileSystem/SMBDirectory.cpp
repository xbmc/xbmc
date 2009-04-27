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

/*
* know bugs:
* - when opening a server for the first time with ip adres and the second time
*   with server name, access to the server is denied.
* - when browsing entire network, user can't go back one step
*   share = smb://, user selects a workgroup, user selects a server.
*   doing ".." will go back to smb:// (entire network) and not to workgroup list.
*
* debugging is set to a max of 10 for release builds (see local.h)
*/

#include "stdafx.h"
#include "SMBDirectory.h"
#include "Util.h"
#include "LocalizeStrings.h"
#include "GUIPassword.h"
#include "lib/libsmb/xbLibSmb.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "GUISettings.h"
#include "FileItem.h"
#include "Settings.h"

struct CachedDirEntry
{
  unsigned int type;
  CStdString name;
};

using namespace DIRECTORY;
using namespace std;

CSMBDirectory::CSMBDirectory(void)
{
} 

CSMBDirectory::~CSMBDirectory(void)
{
}

bool CSMBDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // We accept smb://[[[domain;]user[:password@]]server[/share[/path[/file]]]]

  /* samba isn't thread safe with old interface, always lock */
  CSingleLock lock(smb);

  smb.Init();

  /* we need an url to do proper escaping */
  CURL url(strPath);

  //Separate roots for the authentication and the containing items to allow browsing to work correctly
  CStdString strRoot = strPath;
  CStdString strAuth;

  lock.Leave(); // OpenDir is locked
  int fd = OpenDir(url, strAuth);
  if (fd < 0)
    return false;

  if (!CUtil::HasSlashAtEnd(strRoot)) strRoot += "/";
  if (!CUtil::HasSlashAtEnd(strAuth)) strAuth += "/";

  CStdString strFile;

  // need to keep the samba lock for as short as possible.
  // so we first cache all directory entries and then go over them again asking for stat
  // "stat" is locked each time. that way the lock is freed between stat requests
  vector<CachedDirEntry> vecEntries;
  struct smbc_dirent* dirEnt;

  lock.Enter();
  while ((dirEnt = smbc_readdir(fd)))
  {
    CachedDirEntry aDir;
    aDir.type = dirEnt->smbc_type;
    aDir.name = dirEnt->name;
    vecEntries.push_back(aDir);
  }
  smbc_closedir(fd);
  lock.Leave();

  for (size_t i=0; i<vecEntries.size(); i++)
  {
    CachedDirEntry aDir = vecEntries[i];

    // We use UTF-8 internally, as does SMB
    strFile = aDir.name;

    if (!strFile.Equals(".") && !strFile.Equals("..")
      && aDir.type != SMBC_PRINTER_SHARE && aDir.type != SMBC_IPC_SHARE)
    {
     __int64 iSize = 0;
      bool bIsDir = true;
      __int64 lTimeDate = 0;
      bool hidden = false;

      if(strFile.Right(1).Equals("$") && aDir.type == SMBC_FILE_SHARE )
        continue;

      // only stat files that can give proper responses
      if ( aDir.type == SMBC_FILE ||
           aDir.type == SMBC_DIR )
      {
        // set this here to if the stat should fail
        bIsDir = (aDir.type == SMBC_DIR);

#ifndef _LINUX
        struct __stat64 info = {0};
#else
        struct stat info = {0};
#endif
        if (m_extFileInfo && g_advancedSettings.m_sambastatfiles)
        {
          // make sure we use the authenticated path wich contains any default username
          CStdString strFullName = strAuth + smb.URLEncode(strFile);

          lock.Enter();

          if( smbc_stat(strFullName.c_str(), &info) == 0 )
          {

#ifndef _LINUX
            if ((info.st_mode & S_IXOTH))
              hidden = true;
#else
            char value[20];
            // We poll for extended attributes which symbolizes bits but split up into a string. Where 0x02 is hidden and 0x12 is hidden directory.
            // According to the libsmbclient.h it's supposed to return 0 if ok, or the length of the string. It seems always to return the length wich is 4
            if (smbc_getxattr(strFullName, "system.dos_attr.mode", value, sizeof(value)) > 0)
            {
              long longvalue = strtol(value, NULL, 16);
              if (longvalue & SMBC_DOS_MODE_HIDDEN)
                hidden = true;
            }
            else
              CLog::Log(LOGERROR, "Getting extended attributes for the share: '%s'\nunix_err:'%x' error: '%s'", strFullName.c_str(), errno, strerror(errno));
#endif

            bIsDir = (info.st_mode & S_IFDIR) ? true : false;
            lTimeDate = info.st_mtime;
            if(lTimeDate == 0) // if modification date is missing, use create date
              lTimeDate = info.st_ctime;
            iSize = info.st_size;
          }
          else
            CLog::Log(LOGERROR, "%s - Failed to stat file %s", __FUNCTION__, strFullName.c_str());

          lock.Leave();
        }
      }

      FILETIME fileTime, localTime;
      LONGLONG ll = Int32x32To64(lTimeDate & 0xffffffff, 10000000) + 116444736000000000ll;
      fileTime.dwLowDateTime = (DWORD) (ll & 0xffffffff);
      fileTime.dwHighDateTime = (DWORD)(ll >> 32);
      FileTimeToLocalFileTime(&fileTime, &localTime);

      if (bIsDir)
      {
        CFileItemPtr pItem(new CFileItem(strFile));
        pItem->m_strPath = strRoot;

        // needed for network / workgroup browsing
        // skip if root if we are given a server
        if (aDir.type == SMBC_SERVER)
        {
          /* create url with same options, user, pass.. but no filename or host*/
          CURL rooturl(strRoot);
          rooturl.SetFileName("");
          rooturl.SetHostName("");
          pItem->m_strPath = smb.URLEncode(rooturl);
        }
        pItem->m_strPath += aDir.name;
        if (!CUtil::HasSlashAtEnd(pItem->m_strPath)) pItem->m_strPath += '/';
        pItem->m_bIsFolder = true;
        pItem->m_dateTime=localTime;
        if (hidden)
          pItem->SetProperty("file:hidden", true);
        items.Add(pItem);
      }
      else
      {
        CFileItemPtr pItem(new CFileItem(strFile));
        pItem->m_strPath = strRoot + aDir.name;
        pItem->m_bIsFolder = false;
        pItem->m_dwSize = iSize;
        pItem->m_dateTime=localTime;
        if (hidden)
          pItem->SetProperty("file:hidden", true);
        items.Add(pItem);
      }
    }
  }

  return true;
}

int CSMBDirectory::Open(const CURL &url)
{
  smb.Init();
  CStdString strAuth;
  return OpenDir(url, strAuth);
}

/// \brief Checks authentication against SAMBA share and prompts for username and password if needed
/// \param strAuth The SMB style path
/// \return SMB file descriptor
int CSMBDirectory::OpenDir(const CURL& url, CStdString& strAuth)
{
  int fd = -1;
#ifndef _LINUX
  int nt_error;
#endif

  /* make a writeable copy */
  CURL urlIn(url);

  /* set original url */
  strAuth = smb.URLEncode(urlIn);

  CStdString strPath;
  CStdString strShare;
  /* must url encode this as, auth code will look for the encoded value */
  strShare  = smb.URLEncode(urlIn.GetHostName());
  strShare += "/";
  strShare += smb.URLEncode(urlIn.GetShareName());

  IMAPPASSWORDS it = g_passwordManager.m_mapSMBPasswordCache.find(strShare);
  if(it != g_passwordManager.m_mapSMBPasswordCache.end())
  {
    // if share found in cache use it to supply username and password
    CURL url(it->second);    // map value contains the full url of the originally authenticated share. map key is just the share
    CStdString strPassword = url.GetPassWord();
    CStdString strUserName = url.GetUserName();
    urlIn.SetPassword(strPassword);
    urlIn.SetUserName(strUserName);
  }

  // for a finite number of attempts use the following instead of the while loop:
  // for(int i = 0; i < 3, fd < 0; i++)
  while (fd < 0)
  {
    /* samba has a stricter url encoding, than our own.. CURL can decode it properly */
    /* however doesn't always encode it correctly (spaces for example) */
    strPath = smb.URLEncode(urlIn);

    // remove the / or \ at the end. the samba library does not strip them off
    // don't do this for smb:// !!
    CStdString s = strPath;
    int len = s.length();
    if (len > 1 && s.at(len - 2) != '/' &&
        (s.at(len - 1) == '/' || s.at(len - 1) == '\\'))
    {
      s.erase(len - 1, 1);
    }

    CLog::Log(LOGDEBUG, "%s - Using authentication url %s", __FUNCTION__, s.c_str());
    { CSingleLock lock(smb);
      fd = smbc_opendir(s.c_str());
    }

    if (fd < 0)
    {
#ifndef _LINUX
      nt_error = smb.ConvertUnixToNT(errno);

      // if we have an 'invalid handle' error we don't display the error
      // because most of the time this means there is no cdrom in the server's
      // cdrom drive.
      if (nt_error == NT_STATUS_INVALID_HANDLE)
        break;
#endif

      // NOTE: be sure to warn in XML file about Windows account lock outs when too many attempts
      // if the error is access denied, prompt for a valid user name and password
#ifndef _LINUX
      if (nt_error == NT_STATUS_ACCESS_DENIED)
#else
      if (errno == EACCES)
#endif
      {
        if (m_allowPrompting)
        {
          g_passwordManager.SetSMBShare(strPath);
          if (!g_passwordManager.GetSMBShareUserPassword())  // Do this bit via a threadmessage?
            break;

          /* must do this as our urlencoding for spaces is invalid for samba */
          /* and doing double url encoding will fail */
          /* curl doesn't decode / encode filename yet */
          CURL urlnew( g_passwordManager.GetSMBShare() );
          urlIn.SetUserName(urlnew.GetUserName());
          urlIn.SetPassword(urlnew.GetPassWord());
        }
        else
          break;
      }
      else
      {
        CStdString cError;
#ifndef _LINUX
        if (nt_error == NT_STATUS_OBJECT_NAME_NOT_FOUND)
          cError.Format(g_localizeStrings.Get(770).c_str(),nt_error);
        else
          cError = get_friendly_nt_error_msg(nt_error);
#else
        if (errno == ENODEV || errno == ENOENT)
          cError.Format(g_localizeStrings.Get(770).c_str(),errno);
        else
          cError = strerror(errno);
#endif

        if (m_allowPrompting)
        {
          CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
          pDialog->SetHeading(257);
          pDialog->SetLine(0, cError);
          pDialog->SetLine(1, "");
          pDialog->SetLine(2, "");

          ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
          g_applicationMessenger.SendMessage(tMsg, false);
        }
        break;
      }
    }
  }

  if (fd < 0)
  {
    // write error to logfile
#ifndef _LINUX
    CLog::Log(LOGERROR, "SMBDirectory->GetDirectory: Unable to open directory : '%s'\nunix_err:'%x' nt_err : '%x' error : '%s'", strPath.c_str(), errno, nt_error, get_friendly_nt_error_msg(nt_error));
#else
    CLog::Log(LOGERROR, "SMBDirectory->GetDirectory: Unable to open directory : '%s'\nunix_err:'%x' error : '%s'", strPath.c_str(), errno, strerror(errno));
#endif
  }
  else if (strPath != strAuth && !strShare.IsEmpty()) // we succeeded so, if path was changed, return the correct one and cache it
  {
    g_passwordManager.m_mapSMBPasswordCache[strShare] = strPath;
    strAuth = strPath;
  }

  return fd;
}

bool CSMBDirectory::Create(const char* strPath)
{
  CSingleLock lock(smb);
  smb.Init();

  CURL url(strPath);
  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

  int result = smbc_mkdir(strFileName.c_str(), 0);

  if(result != 0)
#ifndef _LINUX
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
#endif

  return (result == 0 || EEXIST == result);
}

bool CSMBDirectory::Remove(const char* strPath)
{
  CSingleLock lock(smb);
  smb.Init();

  CURL url(strPath);
  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

  int result = smbc_rmdir(strFileName.c_str());

  if(result != 0)
#ifndef _LINUX
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
#endif

  return (result == 0);
}

bool CSMBDirectory::Exists(const char* strPath)
{
  CSingleLock lock(smb);
  smb.Init();

  CURL url(strPath);
  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

#ifndef _LINUX
  SMB_STRUCT_STAT info;
#else
  struct stat info;
#endif
  if (smbc_stat(strFileName.c_str(), &info) != 0)
    return false;

  return (info.st_mode & S_IFDIR) ? true : false;
}
