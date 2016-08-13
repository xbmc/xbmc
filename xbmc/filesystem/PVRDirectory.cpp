/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "PVRDirectory.h"
#include "FileItem.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "guilib/LocalizeStrings.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"

using namespace XFILE;
using namespace PVR;

CPVRDirectory::CPVRDirectory()
{
}

CPVRDirectory::~CPVRDirectory()
{
}

bool CPVRDirectory::Exists(const CURL& url)
{
  if (!g_PVRManager.IsStarted())
    return false;

  return (url.IsProtocol("pvr") && StringUtils::StartsWith(url.GetFileName(), "recordings"));
}

bool CPVRDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string base(url.Get());
  URIUtils::RemoveSlashAtEnd(base);

  std::string fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);
  CLog::Log(LOGDEBUG, "CPVRDirectory::GetDirectory(%s)", base.c_str());
  items.SetCacheToDisc(CFileItemList::CACHE_NEVER);

  if (fileName == "")
  {
    if (!g_PVRManager.IsStarted())
      return false;

    CFileItemPtr item;

    item.reset(new CFileItem(base + "channels/", true));
    item->SetLabel(g_localizeStrings.Get(19019));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "recordings/active/", true));
    item->SetLabel(g_localizeStrings.Get(19017)); // TV Recordings
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "recordings/deleted/", true));
    item->SetLabel(g_localizeStrings.Get(19108)); // Deleted TV Recordings
    item->SetLabelPreformated(true);
    items.Add(item);

    // Sort by name only. Labels are preformated.
    items.AddSortMethod(SortByLabel, 551 /* Name */, LABEL_MASKS("%L", "", "%L", ""));

    return true;
  }
  else if (StringUtils::StartsWith(fileName, "recordings"))
  {
    if (!g_PVRManager.IsStarted())
      return false;

    const std::string pathToUrl(url.Get());
    return g_PVRRecordings->GetDirectory(pathToUrl, items);
  }
  else if (StringUtils::StartsWith(fileName, "channels"))
  {
    if (!g_PVRChannelGroups || !g_PVRChannelGroups->Loaded())
      return false;

    const std::string pathToUrl(url.Get());
    return g_PVRChannelGroups->GetDirectory(pathToUrl, items);
  }
  else if (StringUtils::StartsWith(fileName, "timers"))
  {
    if (!g_PVRManager.IsStarted())
      return false;

    const std::string pathToUrl(url.Get());
    return g_PVRTimers->GetDirectory(pathToUrl, items);
  }

  return false;
}

bool CPVRDirectory::SupportsWriteFileOperations(const std::string& strPath)
{
  CURL url(strPath);
  std::string filename = url.GetFileName();

  return URIUtils::IsPVRRecording(filename);
}

bool CPVRDirectory::IsLiveTV(const std::string& strPath)
{
  CURL url(strPath);
  std::string filename = url.GetFileName();

  return URIUtils::IsLiveTV(filename);
}

bool CPVRDirectory::HasTVRecordings()
{
  return g_PVRManager.IsStarted() ?
    g_PVRRecordings->GetNumTVRecordings() > 0 : false;
}

bool CPVRDirectory::HasDeletedTVRecordings()
{
  return g_PVRManager.IsStarted() ?
    g_PVRRecordings->HasDeletedTVRecordings() : false;
}

bool CPVRDirectory::HasRadioRecordings()
{
  return g_PVRManager.IsStarted() ?
    g_PVRRecordings->GetNumRadioRecordings() > 0 : false;
}

bool CPVRDirectory::HasDeletedRadioRecordings()
{
  return g_PVRManager.IsStarted() ?
    g_PVRRecordings->HasDeletedRadioRecordings() : false;
}
