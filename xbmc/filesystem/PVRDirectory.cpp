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

#ifndef FILESYSTEM_PVRDIRECTORY_H_INCLUDED
#define FILESYSTEM_PVRDIRECTORY_H_INCLUDED
#include "PVRDirectory.h"
#endif

#ifndef FILESYSTEM_FILEITEM_H_INCLUDED
#define FILESYSTEM_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef FILESYSTEM_UTIL_H_INCLUDED
#define FILESYSTEM_UTIL_H_INCLUDED
#include "Util.h"
#endif

#ifndef FILESYSTEM_URL_H_INCLUDED
#define FILESYSTEM_URL_H_INCLUDED
#include "URL.h"
#endif

#ifndef FILESYSTEM_UTILS_LOG_H_INCLUDED
#define FILESYSTEM_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

#ifndef FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif

#ifndef FILESYSTEM_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#define FILESYSTEM_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#include "guilib/LocalizeStrings.h"
#endif


#ifndef FILESYSTEM_PVR_PVRMANAGER_H_INCLUDED
#define FILESYSTEM_PVR_PVRMANAGER_H_INCLUDED
#include "pvr/PVRManager.h"
#endif

#ifndef FILESYSTEM_PVR_CHANNELS_PVRCHANNELGROUPSCONTAINER_H_INCLUDED
#define FILESYSTEM_PVR_CHANNELS_PVRCHANNELGROUPSCONTAINER_H_INCLUDED
#include "pvr/channels/PVRChannelGroupsContainer.h"
#endif

#ifndef FILESYSTEM_PVR_CHANNELS_PVRCHANNELGROUP_H_INCLUDED
#define FILESYSTEM_PVR_CHANNELS_PVRCHANNELGROUP_H_INCLUDED
#include "pvr/channels/PVRChannelGroup.h"
#endif

#ifndef FILESYSTEM_PVR_RECORDINGS_PVRRECORDINGS_H_INCLUDED
#define FILESYSTEM_PVR_RECORDINGS_PVRRECORDINGS_H_INCLUDED
#include "pvr/recordings/PVRRecordings.h"
#endif

#ifndef FILESYSTEM_PVR_TIMERS_PVRTIMERS_H_INCLUDED
#define FILESYSTEM_PVR_TIMERS_PVRTIMERS_H_INCLUDED
#include "pvr/timers/PVRTimers.h"
#endif


using namespace std;
using namespace XFILE;
using namespace PVR;

CPVRDirectory::CPVRDirectory()
{
}

CPVRDirectory::~CPVRDirectory()
{
}

bool CPVRDirectory::Exists(const char* strPath)
{
  CStdString directory(strPath);
  if (directory.substr(0,17) == "pvr://recordings/")
    return true;
  else
    return false;
}

bool CPVRDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString base(strPath);
  URIUtils::RemoveSlashAtEnd(base);

  CURL url(strPath);
  CStdString fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);
  CLog::Log(LOGDEBUG, "CPVRDirectory::GetDirectory(%s)", base.c_str());
  items.SetCacheToDisc(CFileItemList::CACHE_NEVER);

  if (!g_PVRManager.IsStarted())
    return false;

  if (fileName == "")
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/channels/", true));
    item->SetLabel(g_localizeStrings.Get(19019));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/recordings/", true));
    item->SetLabel(g_localizeStrings.Get(19017));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/timers/", true));
    item->SetLabel(g_localizeStrings.Get(19040));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/guide/", true));
    item->SetLabel(g_localizeStrings.Get(19029));
    item->SetLabelPreformated(true);
    items.Add(item);

    // Sort by name only. Labels are preformated.
    items.AddSortMethod(SortByLabel, 551 /* Name */, LABEL_MASKS("%L", "", "%L", ""));

    return true;
  }
  else if (StringUtils::StartsWith(fileName, "recordings"))
  {
    return g_PVRRecordings->GetDirectory(strPath, items);
  }
  else if (StringUtils::StartsWith(fileName, "channels"))
  {
    return g_PVRChannelGroups->GetDirectory(strPath, items);
  }
  else if (StringUtils::StartsWith(fileName, "timers"))
  {
    return g_PVRTimers->GetDirectory(strPath, items);
  }

  return false;
}

bool CPVRDirectory::SupportsWriteFileOperations(const CStdString& strPath)
{
  CURL url(strPath);
  CStdString filename = url.GetFileName();

  return URIUtils::IsPVRRecording(filename);
}

bool CPVRDirectory::IsLiveTV(const CStdString& strPath)
{
  CURL url(strPath);
  CStdString filename = url.GetFileName();

  return URIUtils::IsLiveTV(filename);
}

bool CPVRDirectory::HasRecordings()
{
  return g_PVRRecordings->GetNumRecordings() > 0;
}
