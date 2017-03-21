/*
 *      Copyright (C) 2010-2013 Team XBMC
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
#include "FileUtils.h"
#include "ServiceBroker.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIKeyboardFactory.h"
#include "utils/log.h"
#include "guilib/LocalizeStrings.h"
#include "JobManager.h"
#include "FileOperationJob.h"
#include "URIUtils.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/MultiPathDirectory.h"
#include <vector>
#include "settings/MediaSourceSettings.h"
#include "Util.h"
#include "StringUtils.h"
#include "URL.h"
#include "settings/Settings.h"
#include "utils/Variant.h"

using namespace XFILE;

bool CFileUtils::DeleteItem(const std::string &strPath, bool force)
{
  CFileItemPtr item(new CFileItem(strPath));
  item->SetPath(strPath);
  item->m_bIsFolder = URIUtils::HasSlashAtEnd(strPath);
  item->Select(true);
  return DeleteItem(item, force);
}

bool CFileUtils::DeleteItem(const CFileItemPtr &item, bool force)
{
  if (!item || item->IsParentFolder())
    return false;

  CGUIDialogYesNo* pDialog = g_windowManager.GetWindow<CGUIDialogYesNo>();
  if (!force && pDialog)
  {
    pDialog->SetHeading(CVariant{122});
    pDialog->SetLine(0, CVariant{125});
    pDialog->SetLine(1, CVariant{CURL(item->GetPath()).GetWithoutUserDetails()});
    pDialog->SetLine(2, CVariant{""});
    pDialog->Open();
    if (!pDialog->IsConfirmed()) return false;
  }

  // Create a temporary item list containing the file/folder for deletion
  CFileItemPtr pItemTemp(new CFileItem(*item));
  pItemTemp->Select(true);
  CFileItemList items;
  items.Add(pItemTemp);

  // grab the real filemanager window, set up the progress bar,
  // and process the delete action
  CFileOperationJob op(CFileOperationJob::ActionDelete, items, "");

  return op.DoWork();
}

bool CFileUtils::RenameFile(const std::string &strFile)
{
  std::string strFileAndPath(strFile);
  URIUtils::RemoveSlashAtEnd(strFileAndPath);
  std::string strFileName = URIUtils::GetFileName(strFileAndPath);
  std::string strPath = URIUtils::GetDirectory(strFileAndPath);
  if (CGUIKeyboardFactory::ShowAndGetInput(strFileName, CVariant{g_localizeStrings.Get(16013)}, false))
  {
    strPath = URIUtils::AddFileToFolder(strPath, strFileName);
    CLog::Log(LOGINFO,"FileUtils: rename %s->%s\n", strFileAndPath.c_str(), strPath.c_str());
    if (URIUtils::IsMultiPath(strFileAndPath))
    { // special case for multipath renames - rename all the paths.
      std::vector<std::string> paths;
      CMultiPathDirectory::GetPaths(strFileAndPath, paths);
      bool success = false;
      for (unsigned int i = 0; i < paths.size(); ++i)
      {
        std::string filePath(paths[i]);
        URIUtils::RemoveSlashAtEnd(filePath);
        filePath = URIUtils::GetDirectory(filePath);
        filePath = URIUtils::AddFileToFolder(filePath, strFileName);
        if (CFile::Rename(paths[i], filePath))
          success = true;
      }
      return success;
    }
    return CFile::Rename(strFileAndPath, strPath);
  }
  return false;
}

bool CFileUtils::RemoteAccessAllowed(const std::string &strPath)
{
  const unsigned int SourcesSize = 5;
  std::string SourceNames[] = { "programs", "files", "video", "music", "pictures" };

  std::string realPath = URIUtils::GetRealPath(strPath);
  // for rar:// and zip:// paths we need to extract the path to the archive
  // instead of using the VFS path
  while (URIUtils::IsInArchive(realPath))
    realPath = CURL(realPath).GetHostName();

  if (StringUtils::StartsWithNoCase(realPath, "virtualpath://upnproot/"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "musicdb://"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "videodb://"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "library://video"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "library://music"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "sources://video"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://musicplaylists"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://profile/playlists"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://videoplaylists"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://skin"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://profile/addon_data"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "addons://sources"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "upnp://"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "plugin://"))
    return true;
  else
  {
    std::string strPlaylistsPath = CServiceBroker::GetSettings().GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH);
    URIUtils::RemoveSlashAtEnd(strPlaylistsPath);
    if (StringUtils::StartsWithNoCase(realPath, strPlaylistsPath)) 
      return true;
  }
  bool isSource;
  for (unsigned int index = 0; index < SourcesSize; index++)
  {
    VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources(SourceNames[index]);
    int sourceIndex = CUtil::GetMatchingSource(realPath, *sources, isSource);
    if (sourceIndex >= 0 && sourceIndex < (int)sources->size() && sources->at(sourceIndex).m_iHasLock != 2 && sources->at(sourceIndex).m_allowSharing)
      return true;
  }
  return false;
}

CDateTime CFileUtils::GetModificationDate(const std::string& strFileNameAndPath, const bool& bUseLatestDate)
{
  CDateTime dateAdded;
  if (strFileNameAndPath.empty())
  {
    CLog::Log(LOGDEBUG, "%s empty strFileNameAndPath variable", __FUNCTION__);
    return dateAdded;
  }

  try
  {
    std::string file = strFileNameAndPath;
    if (URIUtils::IsStack(strFileNameAndPath))
      file = CStackDirectory::GetFirstStackedFile(strFileNameAndPath);

    if (URIUtils::IsInArchive(file))
      file = CURL(file).GetHostName();

    // Let's try to get the modification datetime
    struct __stat64 buffer;
    if (CFile::Stat(file, &buffer) == 0 && (buffer.st_mtime != 0 || buffer.st_ctime != 0))
    {
      time_t now = time(NULL);
      time_t addedTime;
      // Prefer the modification time if it's valid
      if (!bUseLatestDate)
      {
        if (buffer.st_mtime != 0 && (time_t)buffer.st_mtime <= now)
          addedTime = (time_t)buffer.st_mtime;
        else
          addedTime = (time_t)buffer.st_ctime;
      }
      // Use the newer of the creation and modification time
      else
      {
        addedTime = std::max((time_t)buffer.st_ctime, (time_t)buffer.st_mtime);
        // if the newer of the two dates is in the future, we try it with the older one
        if (addedTime > now)
          addedTime = std::min((time_t)buffer.st_ctime, (time_t)buffer.st_mtime);
      }

      // make sure the datetime does is not in the future
      if (addedTime <= now)
      {
        struct tm *time;
#ifdef HAVE_LOCALTIME_R
        struct tm result = {};
        time = localtime_r(&addedTime, &result);
#else
        time = localtime(&addedTime);
#endif
        if (time)
          dateAdded = *time;
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to extract modification date for file (%s)", __FUNCTION__, strFileNameAndPath.c_str());
  }
  return dateAdded;
}
