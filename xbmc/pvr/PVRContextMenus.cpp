/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRContextMenus.h"

#include "ContextMenuItem.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClientMenuHooks.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "pvr/guilib/PVRGUIActionsPlayback.h"
#include "pvr/guilib/PVRGUIActionsRecordings.h"
#include "pvr/guilib/PVRGUIActionsTimers.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimersPath.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"

#include <memory>
#include <string>

namespace PVR
{
namespace CONTEXTMENUITEM
{
#define DECL_STATICCONTEXTMENUITEM(clazz) \
  class clazz : public CStaticContextMenuAction \
  { \
  public: \
    explicit clazz(uint32_t label) : CStaticContextMenuAction(label) {} \
    bool IsVisible(const CFileItem& item) const override; \
    bool Execute(const CFileItemPtr& item) const override; \
  };

#define DECL_CONTEXTMENUITEM(clazz) \
  class clazz : public IContextMenuItem \
  { \
  public: \
    std::string GetLabel(const CFileItem& item) const override; \
    bool IsVisible(const CFileItem& item) const override; \
    bool Execute(const CFileItemPtr& item) const override; \
  };

DECL_STATICCONTEXTMENUITEM(PlayEpgTag);
DECL_CONTEXTMENUITEM(PlayEpgTagFromHere);
DECL_STATICCONTEXTMENUITEM(PlayRecording);
DECL_CONTEXTMENUITEM(ShowInformation);
DECL_STATICCONTEXTMENUITEM(ShowChannelGuide);
DECL_STATICCONTEXTMENUITEM(FindSimilar);
DECL_STATICCONTEXTMENUITEM(StartRecording);
DECL_STATICCONTEXTMENUITEM(StopRecording);
DECL_STATICCONTEXTMENUITEM(AddTimerRule);
DECL_CONTEXTMENUITEM(EditTimerRule);
DECL_STATICCONTEXTMENUITEM(DeleteTimerRule);
DECL_CONTEXTMENUITEM(EditTimer);
DECL_CONTEXTMENUITEM(DeleteTimer);
DECL_STATICCONTEXTMENUITEM(EditRecording);
DECL_CONTEXTMENUITEM(DeleteRecording);
DECL_STATICCONTEXTMENUITEM(UndeleteRecording);
DECL_STATICCONTEXTMENUITEM(DeleteWatchedRecordings);
DECL_CONTEXTMENUITEM(ToggleTimerState);
DECL_STATICCONTEXTMENUITEM(AddReminder);
DECL_STATICCONTEXTMENUITEM(ExecuteSearch);
DECL_STATICCONTEXTMENUITEM(EditSearch);
DECL_STATICCONTEXTMENUITEM(RenameSearch);
DECL_STATICCONTEXTMENUITEM(DeleteSearch);

class PVRClientMenuHook : public IContextMenuItem
{
public:
  explicit PVRClientMenuHook(const CPVRClientMenuHook& hook) : m_hook(hook) {}

  std::string GetLabel(const CFileItem& item) const override;
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const CFileItemPtr& item) const override;

  const CPVRClientMenuHook& GetHook() const { return m_hook; }

private:
  const CPVRClientMenuHook m_hook;
};

std::shared_ptr<CPVRTimerInfoTag> GetTimerInfoTagFromItem(const CFileItem& item)
{
  std::shared_ptr<CPVRTimerInfoTag> timer;

  const std::shared_ptr<const CPVREpgInfoTag> epg(item.GetEPGInfoTag());
  if (epg)
    timer = CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(epg);

  if (!timer)
    timer = item.GetPVRTimerInfoTag();

  return timer;
}

///////////////////////////////////////////////////////////////////////////////
// Play epg tag

bool PlayEpgTag::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVREpgInfoTag> epg(item.GetEPGInfoTag());
  if (epg)
    return epg->IsPlayable();

  return false;
}

bool PlayEpgTag::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().PlayEpgTag(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Play epg tag from here

std::string PlayEpgTagFromHere::GetLabel(const CFileItem& item) const
{
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_PVRPLAYBACK_AUTOPLAYNEXTPROGRAMME))
    return g_localizeStrings.Get(19354); /* Play only this programme */

  return g_localizeStrings.Get(19353); /* Play programmes from here */
}

bool PlayEpgTagFromHere::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVREpgInfoTag> epg(item.GetEPGInfoTag());
  if (epg)
    return epg->IsPlayable();

  return false;
}

bool PlayEpgTagFromHere::Execute(const CFileItemPtr& item) const
{
  ContentUtils::PlayMode mode{ContentUtils::PlayMode::PLAY_FROM_HERE};
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_PVRPLAYBACK_AUTOPLAYNEXTPROGRAMME))
    mode = ContentUtils::PlayMode::PLAY_ONLY_THIS;

  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().PlayEpgTag(*item, mode);
}

///////////////////////////////////////////////////////////////////////////////
// Play recording

bool PlayRecording::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRRecording> recording =
      CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(item.GetEPGInfoTag());
  if (recording)
    return !recording->IsDeleted();

  return false;
}

bool PlayRecording::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().PlayRecording(
      *item, true /* bCheckResume */);
}

///////////////////////////////////////////////////////////////////////////////
// Show information (epg, recording)

std::string ShowInformation::GetLabel(const CFileItem& item) const
{
  if (item.GetPVRRecordingInfoTag())
    return g_localizeStrings.Get(19053); /* Recording Information */

  return g_localizeStrings.Get(19047); /* Programme information */
}

bool ShowInformation::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRChannel> channel(item.GetPVRChannelInfoTag());
  if (channel)
    return channel->GetEPGNow().get() != nullptr;

  if (item.HasEPGInfoTag())
    return !item.GetEPGInfoTag()->IsGapTag();

  const std::shared_ptr<const CPVRTimerInfoTag> timer(item.GetPVRTimerInfoTag());
  if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
    return timer->GetEpgInfoTag().get() != nullptr;

  if (item.GetPVRRecordingInfoTag())
    return true;

  return false;
}

bool ShowInformation::Execute(const CFileItemPtr& item) const
{
  if (item->GetPVRRecordingInfoTag())
    return CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().ShowRecordingInfo(*item);

  return CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().ShowEPGInfo(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Show channel guide

bool ShowChannelGuide::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRChannel> channel(item.GetPVRChannelInfoTag());
  if (channel)
    return channel->GetEPGNow().get() != nullptr;

  return false;
}

bool ShowChannelGuide::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().ShowChannelEPG(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Find similar

bool FindSimilar::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRChannel> channel(item.GetPVRChannelInfoTag());
  if (channel)
    return channel->GetEPGNow().get() != nullptr;

  if (item.HasEPGInfoTag())
    return !item.GetEPGInfoTag()->IsGapTag();

  const std::shared_ptr<const CPVRTimerInfoTag> timer(item.GetPVRTimerInfoTag());
  if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
    return timer->GetEpgInfoTag().get() != nullptr;

  const std::shared_ptr<const CPVRRecording> recording(item.GetPVRRecordingInfoTag());
  if (recording)
    return !recording->IsDeleted();

  return false;
}

bool FindSimilar::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().FindSimilar(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Start recording

bool StartRecording::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(item);

  std::shared_ptr<CPVRChannel> channel = item.GetPVRChannelInfoTag();
  if (channel)
    return client && client->GetClientCapabilities().SupportsTimers() &&
           !CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*channel);

  const std::shared_ptr<const CPVREpgInfoTag> epg = item.GetEPGInfoTag();
  if (epg && epg->IsRecordable())
  {
    if (epg->IsGapTag())
    {
      channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelForEpgTag(epg);
      if (channel)
      {
        return client && client->GetClientCapabilities().SupportsTimers() &&
               !CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*channel);
      }
    }
    else
    {
      return client && client->GetClientCapabilities().SupportsTimers() &&
             !CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(epg);
    }
  }
  return false;
}

bool StartRecording::Execute(const CFileItemPtr& item) const
{
  const std::shared_ptr<const CPVREpgInfoTag> epgTag = item->GetEPGInfoTag();
  if (!epgTag || epgTag->IsActive())
  {
    // instant recording
    std::shared_ptr<CPVRChannel> channel;
    if (epgTag)
      channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelForEpgTag(epgTag);

    if (!channel)
      channel = item->GetPVRChannelInfoTag();

    if (channel)
      return CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().SetRecordingOnChannel(channel,
                                                                                           true);
  }

  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().AddTimer(*item, false);
}

///////////////////////////////////////////////////////////////////////////////
// Stop recording

bool StopRecording::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRRecording> recording(item.GetPVRRecordingInfoTag());
  if (recording && recording->IsInProgress())
    return true;

  std::shared_ptr<CPVRChannel> channel = item.GetPVRChannelInfoTag();
  if (channel)
    return CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*channel);

  const std::shared_ptr<const CPVRTimerInfoTag> timer(GetTimerInfoTagFromItem(item));
  if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
    return timer->IsRecording();

  const std::shared_ptr<const CPVREpgInfoTag> epg = item.GetEPGInfoTag();
  if (epg && epg->IsGapTag())
  {
    channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelForEpgTag(epg);
    if (channel)
      return CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*channel);
  }

  return false;
}

bool StopRecording::Execute(const CFileItemPtr& item) const
{
  const std::shared_ptr<const CPVREpgInfoTag> epgTag = item->GetEPGInfoTag();
  if (epgTag && epgTag->IsGapTag())
  {
    // instance recording
    const std::shared_ptr<CPVRChannel> channel =
        CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelForEpgTag(epgTag);
    if (channel)
      return CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().SetRecordingOnChannel(channel,
                                                                                           false);
  }

  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().StopRecording(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Edit recording

bool EditRecording::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRRecording> recording(item.GetPVRRecordingInfoTag());
  if (recording && !recording->IsDeleted() && !recording->IsInProgress())
  {
    return CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().CanEditRecording(item);
  }
  return false;
}

bool EditRecording::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().EditRecording(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Delete recording

std::string DeleteRecording::GetLabel(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRRecording> recording(item.GetPVRRecordingInfoTag());
  if (recording && recording->IsDeleted())
    return g_localizeStrings.Get(19291); /* Delete permanently */

  return g_localizeStrings.Get(117); /* Delete */
}

bool DeleteRecording::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(item);
  if (client && !client->GetClientCapabilities().SupportsRecordingsDelete())
    return false;

  const std::shared_ptr<const CPVRRecording> recording(item.GetPVRRecordingInfoTag());
  if (recording && !recording->IsInProgress())
    return true;

  // recordings folder?
  if (item.m_bIsFolder &&
      CServiceBroker::GetPVRManager().Clients()->AnyClientSupportingRecordingsDelete())
  {
    const CPVRRecordingsPath path(item.GetPath());
    return path.IsValid() && !path.IsRecordingsRoot();
  }

  return false;
}

bool DeleteRecording::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().DeleteRecording(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Undelete recording

bool UndeleteRecording::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRRecording> recording(item.GetPVRRecordingInfoTag());
  if (recording && recording->IsDeleted())
    return true;

  return false;
}

bool UndeleteRecording::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().UndeleteRecording(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Delete watched recordings

bool DeleteWatchedRecordings::IsVisible(const CFileItem& item) const
{
  // recordings folder?
  if (item.m_bIsFolder && !item.IsParentFolder())
    return CPVRRecordingsPath(item.GetPath()).IsValid();

  return false;
}

bool DeleteWatchedRecordings::Execute(const std::shared_ptr<CFileItem>& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().DeleteWatchedRecordings(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Add reminder

bool AddReminder::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVREpgInfoTag> epg = item.GetEPGInfoTag();
  if (epg && !CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(epg) &&
      epg->StartAsLocalTime() > CDateTime::GetCurrentDateTime())
    return true;

  return false;
}

bool AddReminder::Execute(const std::shared_ptr<CFileItem>& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().AddReminder(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Activate / deactivate timer or timer rule

std::string ToggleTimerState::GetLabel(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRTimerInfoTag> timer(item.GetPVRTimerInfoTag());
  if (timer && !timer->IsDisabled())
    return g_localizeStrings.Get(844); /* Deactivate */

  return g_localizeStrings.Get(843); /* Activate */
}

bool ToggleTimerState::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRTimerInfoTag> timer(item.GetPVRTimerInfoTag());
  if (!timer || URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER) ||
      timer->IsBroken())
    return false;

  return timer->GetTimerType()->SupportsEnableDisable();
}

bool ToggleTimerState::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().ToggleTimerState(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Add timer rule

bool AddTimerRule::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVREpgInfoTag> epg = item.GetEPGInfoTag();
  return (epg && !epg->IsGapTag() &&
          !CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(epg));
}

bool AddTimerRule::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().AddTimerRule(*item, true, true);
}

///////////////////////////////////////////////////////////////////////////////
// Edit timer rule

std::string EditTimerRule::GetLabel(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRTimerInfoTag> timer(GetTimerInfoTagFromItem(item));
  if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
  {
    const std::shared_ptr<const CPVRTimerInfoTag> parentTimer(
        CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer));
    if (parentTimer)
    {
      if (!parentTimer->GetTimerType()->IsReadOnly())
        return g_localizeStrings.Get(19243); /* Edit timer rule */
    }
  }

  return g_localizeStrings.Get(19304); /* View timer rule */
}

bool EditTimerRule::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRTimerInfoTag> timer(GetTimerInfoTagFromItem(item));
  if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
    return timer->HasParent();

  return false;
}

bool EditTimerRule::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().EditTimerRule(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Delete timer rule

bool DeleteTimerRule::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRTimerInfoTag> timer(GetTimerInfoTagFromItem(item));
  if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
  {
    const std::shared_ptr<const CPVRTimerInfoTag> parentTimer(
        CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer));
    if (parentTimer)
      return parentTimer->GetTimerType()->AllowsDelete();
  }

  return false;
}

bool DeleteTimerRule::Execute(const CFileItemPtr& item) const
{
  auto& timers = CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>();
  const std::shared_ptr<CFileItem> parentTimer = timers.GetTimerRule(*item);
  if (parentTimer)
    return timers.DeleteTimerRule(*parentTimer);

  return false;
}

///////////////////////////////////////////////////////////////////////////////
// Edit / View timer

std::string EditTimer::GetLabel(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRTimerInfoTag> timer(GetTimerInfoTagFromItem(item));
  if (timer)
  {
    const std::shared_ptr<const CPVRTimerType> timerType = timer->GetTimerType();
    if (item.GetEPGInfoTag())
    {
      if (timerType->IsReminder())
        return g_localizeStrings.Get(timerType->IsReadOnly() ? 829 /* View reminder */
                                                             : 830); /* Edit reminder */
      else
        return g_localizeStrings.Get(timerType->IsReadOnly() ? 19241 /* View timer */
                                                             : 19242); /* Edit timer */
    }
    else
      return g_localizeStrings.Get(timerType->IsReadOnly() ? 21483 : 21450); /* View/Edit */
  }
  return g_localizeStrings.Get(19241); /* View timer */
}

bool EditTimer::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRTimerInfoTag> timer(GetTimerInfoTagFromItem(item));
  return timer && (!item.GetEPGInfoTag() ||
                   !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER));
}

bool EditTimer::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().EditTimer(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Delete timer

std::string DeleteTimer::GetLabel(const CFileItem& item) const
{
  if (item.GetPVRTimerInfoTag())
    return g_localizeStrings.Get(117); /* Delete */

  const std::shared_ptr<const CPVREpgInfoTag> epg = item.GetEPGInfoTag();
  if (epg)
  {
    const std::shared_ptr<const CPVRTimerInfoTag> timer =
        CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(epg);
    if (timer && timer->IsReminder())
      return g_localizeStrings.Get(827); /* Delete reminder */
  }
  return g_localizeStrings.Get(19060); /* Delete timer */
}

bool DeleteTimer::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRTimerInfoTag> timer(GetTimerInfoTagFromItem(item));
  if (timer &&
      (!item.GetEPGInfoTag() ||
       !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER)) &&
      !timer->IsRecording())
    return timer->GetTimerType()->AllowsDelete();

  return false;
}

bool DeleteTimer::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().DeleteTimer(*item);
}

///////////////////////////////////////////////////////////////////////////////
// PVR Client menu hook

std::string PVRClientMenuHook::GetLabel(const CFileItem& item) const
{
  return m_hook.GetLabel();
}

bool PVRClientMenuHook::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<const CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(item);
  if (!client || m_hook.GetAddonId() != client->ID())
    return false;

  if (m_hook.IsAllHook())
    return !item.m_bIsFolder &&
           !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER);
  else if (m_hook.IsEpgHook())
    return item.IsEPG();
  else if (m_hook.IsChannelHook())
    return item.IsPVRChannel();
  else if (m_hook.IsDeletedRecordingHook())
    return item.IsDeletedPVRRecording();
  else if (m_hook.IsRecordingHook())
    return item.IsUsablePVRRecording();
  else if (m_hook.IsTimerHook())
    return item.IsPVRTimer();
  else
    return false;
}

bool PVRClientMenuHook::Execute(const CFileItemPtr& item) const
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*item);
  if (!client)
    return false;

  if (item->IsEPG())
    return client->CallEpgTagMenuHook(m_hook, item->GetEPGInfoTag()) == PVR_ERROR_NO_ERROR;
  else if (item->IsPVRChannel())
    return client->CallChannelMenuHook(m_hook, item->GetPVRChannelInfoTag()) == PVR_ERROR_NO_ERROR;
  else if (item->IsDeletedPVRRecording())
    return client->CallRecordingMenuHook(m_hook, item->GetPVRRecordingInfoTag(), true) ==
           PVR_ERROR_NO_ERROR;
  else if (item->IsUsablePVRRecording())
    return client->CallRecordingMenuHook(m_hook, item->GetPVRRecordingInfoTag(), false) ==
           PVR_ERROR_NO_ERROR;
  else if (item->IsPVRTimer())
    return client->CallTimerMenuHook(m_hook, item->GetPVRTimerInfoTag()) == PVR_ERROR_NO_ERROR;
  else
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// Execute saved search

bool ExecuteSearch::IsVisible(const CFileItem& item) const
{
  return item.HasEPGSearchFilter();
}

bool ExecuteSearch::Execute(const std::shared_ptr<CFileItem>& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().ExecuteSavedSearch(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Edit saved search

bool EditSearch::IsVisible(const CFileItem& item) const
{
  return item.HasEPGSearchFilter();
}

bool EditSearch::Execute(const std::shared_ptr<CFileItem>& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().EditSavedSearch(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Rename saved search

bool RenameSearch::IsVisible(const CFileItem& item) const
{
  return item.HasEPGSearchFilter();
}

bool RenameSearch::Execute(const std::shared_ptr<CFileItem>& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().RenameSavedSearch(*item);
}

///////////////////////////////////////////////////////////////////////////////
// Delete saved search

bool DeleteSearch::IsVisible(const CFileItem& item) const
{
  return item.HasEPGSearchFilter();
}

bool DeleteSearch::Execute(const std::shared_ptr<CFileItem>& item) const
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().DeleteSavedSearch(*item);
}

} // namespace CONTEXTMENUITEM

CPVRContextMenuManager& CPVRContextMenuManager::GetInstance()
{
  static CPVRContextMenuManager instance;
  return instance;
}

CPVRContextMenuManager::CPVRContextMenuManager()
  : m_items({
        std::make_shared<CONTEXTMENUITEM::PlayEpgTag>(19190), /* Play programme */
        std::make_shared<CONTEXTMENUITEM::PlayEpgTagFromHere>(),
        std::make_shared<CONTEXTMENUITEM::PlayRecording>(19687), /* Play recording */
        std::make_shared<CONTEXTMENUITEM::ShowInformation>(),
        std::make_shared<CONTEXTMENUITEM::ShowChannelGuide>(19686), /* Channel guide */
        std::make_shared<CONTEXTMENUITEM::FindSimilar>(19003), /* Find similar */
        std::make_shared<CONTEXTMENUITEM::ToggleTimerState>(),
        std::make_shared<CONTEXTMENUITEM::AddTimerRule>(19061), /* Add timer */
        std::make_shared<CONTEXTMENUITEM::EditTimerRule>(),
        std::make_shared<CONTEXTMENUITEM::DeleteTimerRule>(19295), /* Delete timer rule */
        std::make_shared<CONTEXTMENUITEM::EditTimer>(),
        std::make_shared<CONTEXTMENUITEM::DeleteTimer>(),
        std::make_shared<CONTEXTMENUITEM::StartRecording>(264), /* Record */
        std::make_shared<CONTEXTMENUITEM::StopRecording>(19059), /* Stop recording */
        std::make_shared<CONTEXTMENUITEM::EditRecording>(21450), /* Edit */
        std::make_shared<CONTEXTMENUITEM::DeleteRecording>(),
        std::make_shared<CONTEXTMENUITEM::UndeleteRecording>(19290), /* Undelete */
        std::make_shared<CONTEXTMENUITEM::DeleteWatchedRecordings>(19327), /* Delete watched */
        std::make_shared<CONTEXTMENUITEM::AddReminder>(826), /* Set reminder */
        std::make_shared<CONTEXTMENUITEM::ExecuteSearch>(137), /* Search */
        std::make_shared<CONTEXTMENUITEM::EditSearch>(21450), /* Edit */
        std::make_shared<CONTEXTMENUITEM::RenameSearch>(118), /* Rename */
        std::make_shared<CONTEXTMENUITEM::DeleteSearch>(117), /* Delete */
    })
{
}

void CPVRContextMenuManager::AddMenuHook(const CPVRClientMenuHook& hook)
{
  if (hook.IsSettingsHook())
    return; // settings hooks are not handled using context menus

  const auto item = std::make_shared<CONTEXTMENUITEM::PVRClientMenuHook>(hook);
  m_items.emplace_back(item);
  m_events.Publish(PVRContextMenuEvent(PVRContextMenuEventAction::ADD_ITEM, item));
}

void CPVRContextMenuManager::RemoveMenuHook(const CPVRClientMenuHook& hook)
{
  if (hook.IsSettingsHook())
    return; // settings hooks are not handled using context menus

  for (auto it = m_items.begin(); it < m_items.end(); ++it)
  {
    const CONTEXTMENUITEM::PVRClientMenuHook* cmh =
        dynamic_cast<const CONTEXTMENUITEM::PVRClientMenuHook*>((*it).get());
    if (cmh && cmh->GetHook() == hook)
    {
      m_events.Publish(PVRContextMenuEvent(PVRContextMenuEventAction::REMOVE_ITEM, *it));
      m_items.erase(it);
      return;
    }
  }
}

} // namespace PVR
