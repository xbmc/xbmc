/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "PVRDirectory.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"

using namespace XFILE;
using namespace PVR;

CPVRDirectory::CPVRDirectory() = default;

CPVRDirectory::~CPVRDirectory() = default;

bool CPVRDirectory::Exists(const CURL& url)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
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
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      CFileItemPtr item;

      item.reset(new CFileItem(base + "channels/", true));
      item->SetLabel(g_localizeStrings.Get(19019));
      item->SetLabelPreformatted(true);
      items.Add(item);

      item.reset(new CFileItem(base + "recordings/active/", true));
      item->SetLabel(g_localizeStrings.Get(19017)); // TV Recordings
      item->SetLabelPreformatted(true);
      items.Add(item);

      item.reset(new CFileItem(base + "recordings/deleted/", true));
      item->SetLabel(g_localizeStrings.Get(19108)); // Deleted TV Recordings
      item->SetLabelPreformatted(true);
      items.Add(item);

      // Sort by name only. Labels are preformatted.
      items.AddSortMethod(SortByLabel, 551 /* Name */, LABEL_MASKS("%L", "", "%L", ""));
    }
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "recordings"))
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      const std::string pathToUrl(url.Get());
      return CServiceBroker::GetPVRManager().Recordings()->GetDirectory(pathToUrl, items);
    }
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "channels"))
  {
    if (CServiceBroker::GetPVRManager().ChannelGroups() && CServiceBroker::GetPVRManager().ChannelGroups()->Loaded())
    {
      const std::string pathToUrl(url.Get());
      return CServiceBroker::GetPVRManager().ChannelGroups()->GetDirectory(pathToUrl, items);
    }
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "timers"))
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      const std::string pathToUrl(url.Get());
      return CServiceBroker::GetPVRManager().Timers()->GetDirectory(pathToUrl, items);
    }
    return true;
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
  return CServiceBroker::GetPVRManager().IsStarted() ?
    CServiceBroker::GetPVRManager().Recordings()->GetNumTVRecordings() > 0 : false;
}

bool CPVRDirectory::HasDeletedTVRecordings()
{
  return CServiceBroker::GetPVRManager().IsStarted() ?
    CServiceBroker::GetPVRManager().Recordings()->HasDeletedTVRecordings() : false;
}

bool CPVRDirectory::HasRadioRecordings()
{
  return CServiceBroker::GetPVRManager().IsStarted() ?
    CServiceBroker::GetPVRManager().Recordings()->GetNumRadioRecordings() > 0 : false;
}

bool CPVRDirectory::HasDeletedRadioRecordings()
{
  return CServiceBroker::GetPVRManager().IsStarted() ?
    CServiceBroker::GetPVRManager().Recordings()->HasDeletedRadioRecordings() : false;
}
