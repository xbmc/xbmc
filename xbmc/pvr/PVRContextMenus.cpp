/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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

#include "ContextMenuItem.h"
#include "ServiceBroker.h"
#include "epg/EpgInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimers.h"
#include "utils/URIUtils.h"

#include "PVRContextMenus.h"

namespace PVR
{
  namespace CONTEXTMENUITEM
  {
    #define DECL_STATICCONTEXTMENUITEM(clazz) \
    class clazz : public CStaticContextMenuAction \
    { \
    public: \
      clazz(uint32_t label) : CStaticContextMenuAction(label) {} \
      bool IsVisible(const CFileItem &item) const override; \
      bool Execute(const CFileItemPtr &item) const override; \
    };

    #define DECL_CONTEXTMENUITEM(clazz) \
    class clazz : public IContextMenuItem \
    { \
    public: \
      std::string GetLabel(const CFileItem &item) const override; \
      bool IsVisible(const CFileItem &item) const override; \
      bool Execute(const CFileItemPtr &item) const override; \
    };

    DECL_CONTEXTMENUITEM(ShowInformation);
    DECL_STATICCONTEXTMENUITEM(FindSimilar);
    DECL_STATICCONTEXTMENUITEM(PlayRecording);
    DECL_STATICCONTEXTMENUITEM(StartRecording);
    DECL_STATICCONTEXTMENUITEM(StopRecording);
    DECL_STATICCONTEXTMENUITEM(AddTimerRule);
    DECL_STATICCONTEXTMENUITEM(EditTimerRule);
    DECL_STATICCONTEXTMENUITEM(DeleteTimerRule);
    DECL_CONTEXTMENUITEM(EditTimer);
    DECL_CONTEXTMENUITEM(DeleteTimer);
    DECL_STATICCONTEXTMENUITEM(RenameRecording);
    DECL_CONTEXTMENUITEM(DeleteRecording);
    DECL_STATICCONTEXTMENUITEM(UndeleteRecording);
    DECL_CONTEXTMENUITEM(ToggleTimerState);
    DECL_STATICCONTEXTMENUITEM(RenameTimer);
    DECL_STATICCONTEXTMENUITEM(PVRClientMenuHook);

    CPVRTimerInfoTagPtr GetTimerInfoTagFromItem(const CFileItem &item)
    {
      CPVRTimerInfoTagPtr timer;

      const CPVREpgInfoTagPtr epg(item.GetEPGInfoTag());
      if (epg)
        timer = epg->Timer();

      if (!timer)
        timer = item.GetPVRTimerInfoTag();

      return timer;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Show information (epg, recording)

    std::string ShowInformation::GetLabel(const CFileItem &item) const
    {
      if (item.GetPVRRecordingInfoTag())
        return g_localizeStrings.Get(19053); /* Recording Information */

      return g_localizeStrings.Get(19047); /* Programme information */
    }

    bool ShowInformation::IsVisible(const CFileItem &item) const
    {
      const CPVRChannelPtr channel(item.GetPVRChannelInfoTag());
      if (channel)
        return channel->GetEPGNow().get() != nullptr;

      if (item.GetEPGInfoTag())
        return true;

      const CPVRTimerInfoTagPtr timer(item.GetPVRTimerInfoTag());
      if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        return timer->GetEpgInfoTag().get() != nullptr;

      if (item.GetPVRRecordingInfoTag())
        return true;

      return false;
    }

    bool ShowInformation::Execute(const CFileItemPtr &item) const
    {
      if (item->GetPVRRecordingInfoTag())
        return CServiceBroker::GetPVRManager().GUIActions()->ShowRecordingInfo(item);

      return CServiceBroker::GetPVRManager().GUIActions()->ShowEPGInfo(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Find similar

    bool FindSimilar::IsVisible(const CFileItem &item) const
    {
      const CPVRChannelPtr channel(item.GetPVRChannelInfoTag());
      if (channel)
        return channel->GetEPGNow().get() != nullptr;

      if (item.GetEPGInfoTag())
        return true;

      const CPVRRecordingPtr recording(item.GetPVRRecordingInfoTag());
      if (recording)
        return !recording->IsDeleted();

      return false;
    }

    bool FindSimilar::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->FindSimilar(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Play recording

    bool PlayRecording::IsVisible(const CFileItem &item) const
    {
      CPVRRecordingPtr recording;

      const CPVREpgInfoTagPtr epg(item.GetEPGInfoTag());
      if (epg)
        recording = epg->Recording();

      if (recording)
        return !recording->IsDeleted();

      return false;
    }

    bool PlayRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->PlayRecording(item, true /* bCheckResume */);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Start recording

    bool StartRecording::IsVisible(const CFileItem &item) const
    {
      const CPVRChannelPtr channel(item.GetPVRChannelInfoTag());
      if (channel)
        return CServiceBroker::GetPVRManager().Clients()->SupportsTimers(channel->ClientID()) && !channel->IsRecording();

      const CPVREpgInfoTagPtr epg(item.GetEPGInfoTag());
      if (epg)
        return CServiceBroker::GetPVRManager().Clients()->SupportsTimers() && !epg->Timer() && epg->EndAsLocalTime() > CDateTime::GetCurrentDateTime();

      return false;
    }

    bool StartRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->AddTimer(item, false);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Stop recording

    bool StopRecording::IsVisible(const CFileItem &item) const
    {
      const CPVRChannelPtr channel(item.GetPVRChannelInfoTag());
      if (channel)
        return channel->IsRecording();

      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        return timer->IsRecording();

      return false;
    }

    bool StopRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->StopRecording(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Rename recording

    bool RenameRecording::IsVisible(const CFileItem &item) const
    {
      const CPVRRecordingPtr recording(item.GetPVRRecordingInfoTag());
      if (recording && !recording->IsDeleted())
        return true;

      return false;
    }

    bool RenameRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->RenameRecording(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Delete recording

    std::string DeleteRecording::GetLabel(const CFileItem &item) const
    {
      const CPVRRecordingPtr recording(item.GetPVRRecordingInfoTag());
      if (recording && recording->IsDeleted())
        return g_localizeStrings.Get(19291); /* Delete permanently */

      return g_localizeStrings.Get(117); /* Delete */
    }

    bool DeleteRecording::IsVisible(const CFileItem &item) const
    {
      if (item.GetPVRRecordingInfoTag())
        return true;

      // recordings folder?
      if (item.m_bIsFolder)
      {
        const CPVRRecordingsPath path(item.GetPath());
        return path.IsValid();
      }

      return false;
    }

    bool DeleteRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->DeleteRecording(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Undelete recording

    bool UndeleteRecording::IsVisible(const CFileItem &item) const
    {
      const CPVRRecordingPtr recording(item.GetPVRRecordingInfoTag());
      if (recording && recording->IsDeleted())
        return true;

      return false;
    }

    bool UndeleteRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->UndeleteRecording(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Activate / deactivate timer or timer rule

    std::string ToggleTimerState::GetLabel(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(item.GetPVRTimerInfoTag());
      if (timer && timer->m_state != PVR_TIMER_STATE_DISABLED)
        return g_localizeStrings.Get(844); /* Deactivate */

      return g_localizeStrings.Get(843); /* Activate */
    }

    bool ToggleTimerState::IsVisible(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(item.GetPVRTimerInfoTag());
      if (!timer || URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        return false;

      const CPVRTimerTypePtr timerType(timer->GetTimerType());
      return timerType && timerType->SupportsEnableDisable();
    }

    bool ToggleTimerState::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->ToggleTimerState(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Add timer rule

    bool AddTimerRule::IsVisible(const CFileItem &item) const
    {
      const CPVREpgInfoTagPtr epg(item.GetEPGInfoTag());
      if (epg)
        return CServiceBroker::GetPVRManager().Clients()->SupportsTimers() && !epg->Timer();

      return false;
    }

    bool AddTimerRule::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->AddTimerRule(item, true);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Edit timer rule

    bool EditTimerRule::IsVisible(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        return timer->GetTimerRuleId() != PVR_TIMER_NO_PARENT;

      return false;
    }

    bool EditTimerRule::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->EditTimerRule(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Delete timer rule

    bool DeleteTimerRule::IsVisible(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        return timer->GetTimerRuleId() != PVR_TIMER_NO_PARENT;

      return false;
    }

    bool DeleteTimerRule::Execute(const CFileItemPtr &item) const
    {
      const CFileItemPtr parentTimer(CServiceBroker::GetPVRManager().Timers()->GetTimerRule(item));
      if (parentTimer)
        return CServiceBroker::GetPVRManager().GUIActions()->DeleteTimerRule(parentTimer);

      return false;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Edit / View timer

    std::string EditTimer::GetLabel(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      if (timer)
      {
        const CPVRTimerTypePtr timerType(timer->GetTimerType());
        if (timerType && !timerType->IsReadOnly() && timer->GetTimerRuleId() == PVR_TIMER_NO_PARENT)
        {
          const CPVREpgInfoTagPtr epg(item.GetEPGInfoTag());
          if (epg)
            return g_localizeStrings.Get(19242); /* Edit timer */
          else
            return g_localizeStrings.Get(21450); /* Edit */
        }
      }

      return g_localizeStrings.Get(19241); /* View timer information */
    }

    bool EditTimer::IsVisible(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      if (timer && (!item.GetEPGInfoTag() || !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER)))
      {
        const CPVRTimerTypePtr timerType(timer->GetTimerType());
        return timerType && !timerType->IsReadOnly() && timer->GetTimerRuleId() == PVR_TIMER_NO_PARENT;
      }

      return false;
    }

    bool EditTimer::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->EditTimer(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Rename timer

    bool RenameTimer::IsVisible(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(item.GetPVRTimerInfoTag());
      if (!timer || URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        return false;

      // As epg-based timers will get it's title from the epg tag, they should not be renamable.
      if (timer->IsManual())
      {
        const CPVRTimerTypePtr timerType(timer->GetTimerType());
        if (!timerType->IsReadOnly())
          return true;
      }

      return false;
    }

    bool RenameTimer::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->RenameTimer(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Delete timer

    std::string DeleteTimer::GetLabel(const CFileItem &item) const
    {
      if (item.GetPVRTimerInfoTag())
        return g_localizeStrings.Get(117); /* Delete */

      return g_localizeStrings.Get(19060); /* Delete timer */
    }

    bool DeleteTimer::IsVisible(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      if (timer && (!item.GetEPGInfoTag() || !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER)) && !timer->IsRecording())
      {
        const CPVRTimerTypePtr timerType(timer->GetTimerType());
        return  timerType && !timerType->IsReadOnly();
      }

      return false;
    }

    bool DeleteTimer::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->DeleteTimer(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // PVR Client menu hook

    bool PVRClientMenuHook::IsVisible(const CFileItem &item) const
    {
      const CPVRChannelPtr channel(item.GetPVRChannelInfoTag());
      if (channel)
        return CServiceBroker::GetPVRManager().Clients()->HasMenuHooks(channel->ClientID(), PVR_MENUHOOK_CHANNEL);

      const CPVREpgInfoTagPtr epg(item.GetEPGInfoTag());
      if (epg)
      {
        const CPVRChannelPtr channel(epg->ChannelTag());
        return (channel && CServiceBroker::GetPVRManager().Clients()->HasMenuHooks(channel->ClientID(), PVR_MENUHOOK_EPG));
      }

      const CPVRTimerInfoTagPtr timer(item.GetPVRTimerInfoTag());
      if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        return CServiceBroker::GetPVRManager().Clients()->HasMenuHooks(timer->m_iClientId, PVR_MENUHOOK_TIMER);

      const CPVRRecordingPtr recording(item.GetPVRRecordingInfoTag());
      if (recording)
      {
        if (recording->IsDeleted())
          return CServiceBroker::GetPVRManager().Clients()->HasMenuHooks(recording->m_iClientId, PVR_MENUHOOK_DELETED_RECORDING);
        else
          return CServiceBroker::GetPVRManager().Clients()->HasMenuHooks(recording->m_iClientId, PVR_MENUHOOK_RECORDING);
      }

      return false;
    }

    bool PVRClientMenuHook::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->ProcessMenuHooks(item);
    }

  } // namespace CONEXTMENUITEM

  CPVRContextMenuManager& CPVRContextMenuManager::GetInstance()
  {
    static CPVRContextMenuManager instance;
    return instance;
  }

  CPVRContextMenuManager::CPVRContextMenuManager()
  {
    m_items =
    {
      std::make_shared<CONTEXTMENUITEM::ShowInformation>(),
      std::make_shared<CONTEXTMENUITEM::FindSimilar>(19003), /* Find similar */
      std::make_shared<CONTEXTMENUITEM::PlayRecording>(19687), /* Play recording */
      std::make_shared<CONTEXTMENUITEM::ToggleTimerState>(),
      std::make_shared<CONTEXTMENUITEM::AddTimerRule>(19061), /* Add timer */
      std::make_shared<CONTEXTMENUITEM::EditTimerRule>(19243), /* Edit timer rule */
      std::make_shared<CONTEXTMENUITEM::DeleteTimerRule>(19295), /* Delete timer rule */
      std::make_shared<CONTEXTMENUITEM::EditTimer>(),
      std::make_shared<CONTEXTMENUITEM::RenameTimer>(118), /* Rename */
      std::make_shared<CONTEXTMENUITEM::DeleteTimer>(),
      std::make_shared<CONTEXTMENUITEM::StartRecording>(264), /* Record */
      std::make_shared<CONTEXTMENUITEM::StopRecording>(19059), /* Stop recording */
      std::make_shared<CONTEXTMENUITEM::RenameRecording>(118), /* Rename */
      std::make_shared<CONTEXTMENUITEM::DeleteRecording>(),
      std::make_shared<CONTEXTMENUITEM::UndeleteRecording>(19290), /* Undelete */
      std::make_shared<CONTEXTMENUITEM::PVRClientMenuHook>(19195), /* PVR client specific action */
    };
  }

} // namespace PVR
