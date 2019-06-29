/*
 *  Copyright (C) 2012-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIDirectory.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimersPath.h"

using namespace PVR;

bool CPVRGUIDirectory::Exists() const
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return false;

  return m_url.IsProtocol("pvr") && StringUtils::StartsWith(m_url.GetFileName(), "recordings");
}

bool CPVRGUIDirectory::SupportsWriteFileOperations() const
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return false;

  const std::string filename = m_url.GetFileName();
  return URIUtils::IsPVRRecording(filename);
}

bool CPVRGUIDirectory::GetDirectory(CFileItemList& results) const
{
  std::string base = m_url.Get();
  URIUtils::RemoveSlashAtEnd(base);

  std::string fileName = m_url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  results.SetCacheToDisc(CFileItemList::CACHE_NEVER);

  if (fileName.empty())
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      std::shared_ptr<CFileItem> item;

      item.reset(new CFileItem(base + "channels/", true));
      item->SetLabel(g_localizeStrings.Get(19019));
      item->SetLabelPreformatted(true);
      results.Add(item);

      item.reset(new CFileItem(base + "recordings/active/", true));
      item->SetLabel(g_localizeStrings.Get(19017)); // TV Recordings
      item->SetLabelPreformatted(true);
      results.Add(item);

      item.reset(new CFileItem(base + "recordings/deleted/", true));
      item->SetLabel(g_localizeStrings.Get(19108)); // Deleted TV Recordings
      item->SetLabelPreformatted(true);
      results.Add(item);

      // Sort by name only. Labels are preformatted.
      results.AddSortMethod(SortByLabel, 551 /* Name */, LABEL_MASKS("%L", "", "%L", ""));
    }
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "recordings"))
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      return GetRecordingsDirectory(results);
    }
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "channels"))
  {
    if (CServiceBroker::GetPVRManager().ChannelGroups() &&
        CServiceBroker::GetPVRManager().ChannelGroups()->Loaded())
    {
      return GetChannelsDirectory(results);
    }
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "timers"))
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      return GetTimersDirectory(results);
    }
    return true;
  }

  return false;
}

bool CPVRGUIDirectory::HasTVRecordings()
{
  return CServiceBroker::GetPVRManager().IsStarted() &&
         CServiceBroker::GetPVRManager().Recordings()->GetNumTVRecordings() > 0;
}

bool CPVRGUIDirectory::HasDeletedTVRecordings()
{
  return CServiceBroker::GetPVRManager().IsStarted() &&
         CServiceBroker::GetPVRManager().Recordings()->HasDeletedTVRecordings();
}

bool CPVRGUIDirectory::HasRadioRecordings()
{
  return CServiceBroker::GetPVRManager().IsStarted() &&
         CServiceBroker::GetPVRManager().Recordings()->GetNumRadioRecordings() > 0;
}

bool CPVRGUIDirectory::HasDeletedRadioRecordings()
{
  return CServiceBroker::GetPVRManager().IsStarted() &&
         CServiceBroker::GetPVRManager().Recordings()->HasDeletedRadioRecordings();
}

namespace
{

std::string TrimSlashes(const std::string& strOrig)
{
  std::string strReturn = strOrig;
  while (strReturn[0] == '/')
    strReturn.erase(0, 1);

  URIUtils::RemoveSlashAtEnd(strReturn);
  return strReturn;
}

bool IsDirectoryMember(const std::string& strDirectory,
                       const std::string& strEntryDirectory,
                       bool bGrouped)
{
  const std::string strUseDirectory = TrimSlashes(strDirectory);
  const std::string strUseEntryDirectory = TrimSlashes(strEntryDirectory);

  // Case-insensitive comparison since sub folders are created with case-insensitive matching (GetSubDirectories)
  if (bGrouped)
    return StringUtils::EqualsNoCase(strUseDirectory, strUseEntryDirectory);
  else
    return StringUtils::StartsWithNoCase(strUseEntryDirectory, strUseDirectory);
}

void GetSubDirectories(const CPVRRecordingsPath& recParentPath,
                       const std::vector<std::shared_ptr<CPVRRecording>>& recordings,
                       CFileItemList& results)
{
  // Only active recordings are fetched to provide sub directories.
  // Not applicable for deleted view which is supposed to be flattened.
  std::set<std::shared_ptr<CFileItem>> unwatchedFolders;
  bool bRadio = recParentPath.IsRadio();

  for (const auto& recording : recordings)
  {
    if (recording->IsDeleted())
      continue;

    if (recording->IsRadio() != bRadio)
      continue;

    const std::string strCurrent = recParentPath.GetUnescapedSubDirectoryPath(recording->m_strDirectory);
    if (strCurrent.empty())
      continue;

    CPVRRecordingsPath recChildPath(recParentPath);
    recChildPath.AppendSegment(strCurrent);
    const std::string strFilePath = recChildPath;

    std::shared_ptr<CFileItem> item;
    if (!results.Contains(strFilePath))
    {
      item.reset(new CFileItem(strCurrent, true));
      item->SetPath(strFilePath);
      item->SetLabel(strCurrent);
      item->SetLabelPreformatted(true);
      item->m_dateTime = recording->RecordingTimeAsLocalTime();

      // Assume all folders are watched, we'll change the overlay later
      item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_WATCHED, false);
      results.Add(item);
    }
    else
    {
      item = results.Get(strFilePath);
      if (item->m_dateTime < recording->RecordingTimeAsLocalTime())
        item->m_dateTime = recording->RecordingTimeAsLocalTime();
    }

    if (recording->GetPlayCount() == 0)
      unwatchedFolders.insert(item);
  }

  // Change the watched overlay to unwatched for folders containing unwatched entries
  for (auto& item : unwatchedFolders)
    item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, false);
}

} // unnamed namespace

bool CPVRGUIDirectory::GetRecordingsDirectory(CFileItemList& results) const
{
  bool bGrouped = false;
  const std::vector<std::shared_ptr<CPVRRecording>> recordings = CServiceBroker::GetPVRManager().Recordings()->GetAll();

  if (m_url.HasOption("view"))
  {
    const std::string view = m_url.GetOption("view");
    if (view == "grouped")
      bGrouped = true;
    else if (view == "flat")
      bGrouped = false;
    else
    {
      CLog::LogF(LOGERROR, "Unsupported value '%s' for url parameter 'view'", view.c_str());
      return false;
    }
  }
  else
  {
    bGrouped = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRRECORD_GROUPRECORDINGS);
  }

  const CPVRRecordingsPath recPath(m_url.GetWithoutOptions());
  if (recPath.IsValid())
  {
    // Get the directory structure if in non-flatten mode
    // Deleted view is always flatten. So only for an active view
    const std::string strDirectory = recPath.GetUnescapedDirectoryPath();
    if (!recPath.IsDeleted() && bGrouped)
      GetSubDirectories(recPath, recordings, results);

    // get all files of the current directory or recursively all files starting at the current directory if in flatten mode
    std::shared_ptr<CFileItem> item;
    for (const auto& recording : recordings)
    {
      // Omit recordings not matching criteria
      if (recording->IsDeleted() != recPath.IsDeleted() ||
          recording->IsRadio() != recPath.IsRadio() ||
          !IsDirectoryMember(strDirectory, recording->m_strDirectory, bGrouped))
        continue;

      item = std::make_shared<CFileItem>(recording);
      item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, recording->GetPlayCount() > 0);
      results.Add(item);
    }
  }

  return recPath.IsValid();
}

bool CPVRGUIDirectory::FilterDirectory(CFileItemList& results) const
{
  if (!results.IsEmpty())
  {
    if (m_url.HasOption("view"))
    {
      const std::string view = m_url.GetOption("view");
      if (view == "lastplayed")
      {
        // remove channels never played so far
        for (int i = 0; i < results.Size(); ++i)
        {
          const std::shared_ptr<CPVRChannel> channel = results.Get(i)->GetPVRChannelInfoTag();
          time_t lastWatched = channel->LastWatched();
          if (!lastWatched)
          {
            results.Remove(i);
            --i;
          }
        }
      }
      else
      {
        CLog::LogF(LOGERROR, "Unsupported value '%s' for channel list URL parameter 'view'", view.c_str());
        return false;
      }
    }
  }
  return true;
}

bool CPVRGUIDirectory::GetChannelGroupsDirectory(bool bRadio, CFileItemList& results) const
{
  const CPVRChannelGroups* channelGroups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(bRadio);
  if (channelGroups)
  {
    channelGroups->GetGroupList(&results);
    return true;
  }
  return false;
}

bool CPVRGUIDirectory::GetChannelsDirectory(CFileItemList& results) const
{
  std::string base = m_url.Get();
  URIUtils::RemoveSlashAtEnd(base);

  std::string fileName = m_url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  if (fileName == "channels")
  {
    std::shared_ptr<CFileItem> item;

    // all tv channels
    item.reset(new CFileItem(base + "/tv/", true));
    item->SetLabel(g_localizeStrings.Get(19020));
    item->SetLabelPreformatted(true);
    results.Add(item);

    // all radio channels
    item.reset(new CFileItem(base + "/radio/", true));
    item->SetLabel(g_localizeStrings.Get(19021));
    item->SetLabelPreformatted(true);
    results.Add(item);

    return true;
  }
  else if (fileName == "channels/tv")
  {
    return GetChannelGroupsDirectory(false, results);
  }
  else if (fileName == "channels/radio")
  {
    return GetChannelGroupsDirectory(true, results);
  }
  else if (StringUtils::StartsWith(fileName, "channels/tv/"))
  {
    std::string strGroupName = fileName.substr(12);
    URIUtils::RemoveSlashAtEnd(strGroupName);

    std::shared_ptr<CPVRChannelGroup> group;
    bool bShowHiddenChannels = StringUtils::EndsWithNoCase(fileName, ".hidden");
    if (bShowHiddenChannels || strGroupName == "*") // all channels
      group = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllTV();
    else
      group = CServiceBroker::GetPVRManager().ChannelGroups()->GetTV()->GetByName(strGroupName);

    if (group)
    {
      const std::vector<PVRChannelGroupMember> groupMembers = group->GetMembers();
      for (const auto& groupMember : groupMembers)
      {
        if (bShowHiddenChannels != groupMember.channel->IsHidden())
          continue;

        results.Add(std::make_shared<CFileItem>(groupMember.channel));
      }
    }
    else
    {
      CLog::LogF(LOGERROR, "Unable to obtain members of channel group '%s'", strGroupName.c_str());
      return false;
    }

    FilterDirectory(results);
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "channels/radio/"))
  {
    std::string strGroupName = fileName.substr(15);
    URIUtils::RemoveSlashAtEnd(strGroupName);

    std::shared_ptr<CPVRChannelGroup> group;
    bool bShowHiddenChannels = StringUtils::EndsWithNoCase(fileName, ".hidden");
    if (bShowHiddenChannels || strGroupName == "*") // all channels
      group = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllRadio();
    else
      group = CServiceBroker::GetPVRManager().ChannelGroups()->GetRadio()->GetByName(strGroupName);

    if (group)
    {
      const std::vector<PVRChannelGroupMember> groupMembers = group->GetMembers();
      for (const auto& groupMember : groupMembers)
      {
        if (bShowHiddenChannels != groupMember.channel->IsHidden())
          continue;

        results.Add(std::make_shared<CFileItem>(groupMember.channel));
      }
    }
    else
    {
      CLog::LogF(LOGERROR, "Unable to obtain members of channel group '%s'", strGroupName.c_str());
      return false;
    }

    FilterDirectory(results);
    return true;
  }

  return false;
}

namespace
{

bool GetTimersRootDirectory(const CPVRTimersPath& path,
                            const std::vector<std::shared_ptr<CPVRTimerInfoTag>>& timers,
                            CFileItemList& results)
{
  std::shared_ptr<CFileItem> item(new CFileItem(CPVRTimersPath::PATH_ADDTIMER, false));
  item->SetLabel(g_localizeStrings.Get(19026)); // "Add timer..."
  item->SetLabelPreformatted(true);
  item->SetSpecialSort(SortSpecialOnTop);
  item->SetIconImage("DefaultTVShows.png");
  results.Add(item);

  bool bRadio = path.IsRadio();
  bool bRules = path.IsRules();

  bool bHideDisabled = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS);

  for (const auto& timer : timers)
  {
    if ((bRadio == timer->m_bIsRadio || (bRules && timer->m_iClientChannelUid == PVR_TIMER_ANY_CHANNEL)) &&
        (bRules == timer->IsTimerRule()) &&
        (!bHideDisabled || (timer->m_state != PVR_TIMER_STATE_DISABLED)))
    {
      item.reset(new CFileItem(timer));
      const CPVRTimersPath timersPath(path.GetPath(), timer->m_iClientId, timer->m_iClientIndex);
      item->SetPath(timersPath.GetPath());
      results.Add(item);
    }
  }
  return true;
}

bool GetTimersSubDirectory(const CPVRTimersPath& path,
                           const std::vector<std::shared_ptr<CPVRTimerInfoTag>>& timers,
                           CFileItemList& results)
{
  bool bRadio = path.IsRadio();
  unsigned int iParentId = path.GetParentId();
  int iClientId = path.GetClientId();

  bool bHideDisabled = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS);

  std::shared_ptr<CFileItem> item;

  for (const auto& timer : timers)
  {
    if ((timer->m_bIsRadio == bRadio) &&
        (timer->m_iParentClientIndex != PVR_TIMER_NO_PARENT) &&
        (timer->m_iClientId == iClientId) &&
        (timer->m_iParentClientIndex == iParentId) &&
        (!bHideDisabled || (timer->m_state != PVR_TIMER_STATE_DISABLED)))
    {
      item.reset(new CFileItem(timer));
      const CPVRTimersPath timersPath(path.GetPath(), timer->m_iClientId, timer->m_iClientIndex);
      item->SetPath(timersPath.GetPath());
      results.Add(item);
    }
  }
  return true;
}

} // unnamed namespace

bool CPVRGUIDirectory::GetTimersDirectory(CFileItemList& results) const
{
  const CPVRTimersPath path(m_url.GetWithoutOptions());
  if (path.IsValid())
  {
    const std::vector<std::shared_ptr<CPVRTimerInfoTag>> timers = CServiceBroker::GetPVRManager().Timers()->GetAll();

    if (path.IsTimersRoot())
    {
      /* Root folder containing either timer rules or timers. */
      return GetTimersRootDirectory(path, timers, results);
    }
    else if (path.IsTimerRule())
    {
      /* Sub folder containing the timers scheduled by the given timer rule. */
      return GetTimersSubDirectory(path, timers, results);
    }
  }

  return false;
}
