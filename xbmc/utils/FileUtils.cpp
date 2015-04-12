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
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIKeyboardFactory.h"
#include "utils/log.h"
#include "guilib/LocalizeStrings.h"
#include "JobManager.h"
#include "FileOperationJob.h"
#include "URIUtils.h"
#include "filesystem/MultiPathDirectory.h"
#include <vector>
#include "settings/MediaSourceSettings.h"
#include "Util.h"
#include "StringUtils.h"
#include "URL.h"
#include "settings/Settings.h"

using namespace XFILE;
using namespace std;

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

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!force && pDialog)
  {
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 125);
    pDialog->SetLine(1, CURL(item->GetPath()).GetWithoutUserDetails());
    pDialog->SetLine(2, "");
    pDialog->DoModal();
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
  if (CGUIKeyboardFactory::ShowAndGetInput(strFileName, g_localizeStrings.Get(16013), false))
  {
    strPath = URIUtils::AddFileToFolder(strPath, strFileName);
    CLog::Log(LOGINFO,"FileUtils: rename %s->%s\n", strFileAndPath.c_str(), strPath.c_str());
    if (URIUtils::IsMultiPath(strFileAndPath))
    { // special case for multipath renames - rename all the paths.
      vector<std::string> paths;
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

  string realPath = URIUtils::GetRealPath(strPath);
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
    std::string strPlaylistsPath = CSettings::Get().GetString("system.playlistspath");
    URIUtils::RemoveSlashAtEnd(strPlaylistsPath);
    if (StringUtils::StartsWithNoCase(realPath, strPlaylistsPath)) 
      return true;
  }
  bool isSource;
  for (unsigned int index = 0; index < SourcesSize; index++)
  {
    VECSOURCES* sources = CMediaSourceSettings::Get().GetSources(SourceNames[index]);
    int sourceIndex = CUtil::GetMatchingSource(realPath, *sources, isSource);
    if (sourceIndex >= 0 && sourceIndex < (int)sources->size() && sources->at(sourceIndex).m_iHasLock != 2 && sources->at(sourceIndex).m_allowSharing)
      return true;
  }
  return false;
}
