/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsDatabase.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/guilib/PVRGUIActionsParentalControl.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <memory>
#include <string>

using namespace PVR;

namespace
{
class CPVRGUIDatabaseResetComponentsSelector
{
public:
  CPVRGUIDatabaseResetComponentsSelector() = default;
  virtual ~CPVRGUIDatabaseResetComponentsSelector() = default;

  bool Select()
  {
    CGUIDialogSelect* pDlgSelect =
        CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
            WINDOW_DIALOG_SELECT);
    if (!pDlgSelect)
    {
      CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT!");
      return false;
    }

    CFileItemList options;

    const std::shared_ptr<CFileItem> itemAll =
        std::make_shared<CFileItem>(StringUtils::Format(g_localizeStrings.Get(593))); // All
    itemAll->SetPath("all");
    options.Add(itemAll);

    // if channels are cleared, groups, EPG data and providers must also be cleared
    const std::shared_ptr<CFileItem> itemChannels =
        std::make_shared<CFileItem>(StringUtils::Format("{}, {}, {}, {}",
                                                        g_localizeStrings.Get(19019), // Channels
                                                        g_localizeStrings.Get(19146), // Groups
                                                        g_localizeStrings.Get(19069), // Guide
                                                        g_localizeStrings.Get(19334))); // Providers
    itemChannels->SetPath("channels");
    itemChannels->Select(true); // preselect this item in dialog
    options.Add(itemChannels);

    const std::shared_ptr<CFileItem> itemGroups =
        std::make_shared<CFileItem>(g_localizeStrings.Get(19146)); // Groups
    itemGroups->SetPath("groups");
    options.Add(itemGroups);

    const std::shared_ptr<CFileItem> itemGuide =
        std::make_shared<CFileItem>(g_localizeStrings.Get(19069)); // Guide
    itemGuide->SetPath("guide");
    options.Add(itemGuide);

    const std::shared_ptr<CFileItem> itemProviders =
        std::make_shared<CFileItem>(g_localizeStrings.Get(19334)); // Providers
    itemProviders->SetPath("providers");
    options.Add(itemProviders);

    const std::shared_ptr<CFileItem> itemReminders =
        std::make_shared<CFileItem>(g_localizeStrings.Get(19215)); // Reminders
    itemReminders->SetPath("reminders");
    options.Add(itemReminders);

    const std::shared_ptr<CFileItem> itemRecordings =
        std::make_shared<CFileItem>(g_localizeStrings.Get(19017)); // Recordings
    itemRecordings->SetPath("recordings");
    options.Add(itemRecordings);

    const std::shared_ptr<CFileItem> itemClients =
        std::make_shared<CFileItem>(g_localizeStrings.Get(24019)); // PVR clients
    itemClients->SetPath("clients");
    options.Add(itemClients);

    pDlgSelect->Reset();
    pDlgSelect->SetHeading(CVariant{g_localizeStrings.Get(19185)}); // "Clear data"
    pDlgSelect->SetItems(options);
    pDlgSelect->SetMultiSelection(true);
    pDlgSelect->Open();

    if (!pDlgSelect->IsConfirmed())
      return false;

    for (int i : pDlgSelect->GetSelectedItems())
    {
      const std::string path = options.Get(i)->GetPath();

      m_bResetChannels |= (path == "channels" || path == "all");
      m_bResetGroups |= (path == "groups" || path == "all");
      m_bResetGuide |= (path == "guide" || path == "all");
      m_bResetProviders |= (path == "providers" || path == "all");
      m_bResetReminders |= (path == "reminders" || path == "all");
      m_bResetRecordings |= (path == "recordings" || path == "all");
      m_bResetClients |= (path == "clients" || path == "all");
    }

    m_bResetGroups |= m_bResetChannels;
    m_bResetGuide |= m_bResetChannels;
    m_bResetProviders |= m_bResetChannels;

    return (m_bResetChannels || m_bResetGroups || m_bResetGuide || m_bResetProviders ||
            m_bResetReminders || m_bResetRecordings || m_bResetClients);
  }

  bool IsResetChannelsSelected() const { return m_bResetChannels; }
  bool IsResetGroupsSelected() const { return m_bResetGroups; }
  bool IsResetGuideSelected() const { return m_bResetGuide; }
  bool IsResetProvidersSelected() const { return m_bResetProviders; }
  bool IsResetRemindersSelected() const { return m_bResetReminders; }
  bool IsResetRecordingsSelected() const { return m_bResetRecordings; }
  bool IsResetClientsSelected() const { return m_bResetClients; }

private:
  bool m_bResetChannels = false;
  bool m_bResetGroups = false;
  bool m_bResetGuide = false;
  bool m_bResetProviders = false;
  bool m_bResetReminders = false;
  bool m_bResetRecordings = false;
  bool m_bResetClients = false;
};

} // unnamed namespace

bool CPVRGUIActionsDatabase::ResetDatabase(bool bResetEPGOnly)
{
  CGUIDialogProgress* pDlgProgress =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(
          WINDOW_DIALOG_PROGRESS);
  if (!pDlgProgress)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PROGRESS!");
    return false;
  }

  bool bResetChannels = false;
  bool bResetGroups = false;
  bool bResetGuide = false;
  bool bResetProviders = false;
  bool bResetReminders = false;
  bool bResetRecordings = false;
  bool bResetClients = false;

  if (bResetEPGOnly)
  {
    if (!CGUIDialogYesNo::ShowAndGetInput(
            CVariant{19098}, // "Warning!"
            CVariant{19188})) // "All guide data will be cleared. Are you sure?"
      return false;

    bResetGuide = true;
  }
  else
  {
    if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Parental>().CheckParentalPIN() !=
        ParentalCheckResult::SUCCESS)
      return false;

    CPVRGUIDatabaseResetComponentsSelector selector;
    if (!selector.Select())
      return false;

    if (!CGUIDialogYesNo::ShowAndGetInput(
            CVariant{19098}, // "Warning!"
            CVariant{19186})) // "All selected data will be cleared. ... Are you sure?"
      return false;

    bResetChannels = selector.IsResetChannelsSelected();
    bResetGroups = selector.IsResetGroupsSelected();
    bResetGuide = selector.IsResetGuideSelected();
    bResetProviders = selector.IsResetProvidersSelected();
    bResetReminders = selector.IsResetRemindersSelected();
    bResetRecordings = selector.IsResetRecordingsSelected();
    bResetClients = selector.IsResetClientsSelected();
  }

  CDateTime::ResetTimezoneBias();

  CLog::LogFC(LOGDEBUG, LOGPVR, "PVR clearing {} database", bResetEPGOnly ? "EPG" : "PVR and EPG");

  pDlgProgress->SetHeading(CVariant{313}); // "Cleaning database"
  pDlgProgress->SetLine(0, CVariant{g_localizeStrings.Get(19187)}); // "Clearing all related data."
  pDlgProgress->SetLine(1, CVariant{""});
  pDlgProgress->SetLine(2, CVariant{""});

  pDlgProgress->Open();
  pDlgProgress->Progress();

  if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlaying())
  {
    CLog::Log(LOGINFO, "PVR is stopping playback for {} database reset",
              bResetEPGOnly ? "EPG" : "PVR and EPG");
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_STOP);
  }

  const std::shared_ptr<CPVRDatabase> pvrDatabase(CServiceBroker::GetPVRManager().GetTVDatabase());
  const std::shared_ptr<CPVREpgDatabase> epgDatabase(
      CServiceBroker::GetPVRManager().EpgContainer().GetEpgDatabase());

  // increase db open refcounts, so they don't get closed during following pvr manager shutdown
  pvrDatabase->Open();
  epgDatabase->Open();

  // stop pvr manager; close both pvr and epg databases
  CServiceBroker::GetPVRManager().Stop();

  const int iProgressStepPercentage =
      100 / ((2 * bResetChannels) + bResetGroups + bResetGuide + bResetProviders + bResetReminders +
             bResetRecordings + bResetClients + 1);
  int iProgressStepsDone = 0;

  if (bResetProviders)
  {
    pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
    pDlgProgress->Progress();

    // delete all providers
    pvrDatabase->DeleteProviders();
  }

  if (bResetGuide)
  {
    pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
    pDlgProgress->Progress();

    // reset channel's EPG pointers
    pvrDatabase->ResetEPG();

    // delete all entries from the EPG database
    epgDatabase->DeleteEpg();
  }

  if (bResetGroups)
  {
    pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
    pDlgProgress->Progress();

    // delete all channel groups (including data only available locally, like user defined groups)
    pvrDatabase->DeleteChannelGroups();
  }

  if (bResetChannels)
  {
    pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
    pDlgProgress->Progress();

    // delete all channels (including data only available locally, like user set icons)
    pvrDatabase->DeleteChannels();
  }

  if (bResetReminders)
  {
    pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
    pDlgProgress->Progress();

    // delete all timers data (e.g. all reminders, which are only stored locally)
    pvrDatabase->DeleteTimers();
  }

  if (bResetClients)
  {
    pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
    pDlgProgress->Progress();

    // delete all clients data (e.g priorities, which are only stored locally)
    pvrDatabase->DeleteClients();
  }

  if (bResetChannels || bResetRecordings)
  {
    CVideoDatabase videoDatabase;

    if (videoDatabase.Open())
    {
      if (bResetChannels)
      {
        pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
        pDlgProgress->Progress();

        // delete all channel's entries (e.g. settings, bookmarks, stream details)
        videoDatabase.EraseAllForPath("pvr://channels/");
      }

      if (bResetRecordings)
      {
        pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
        pDlgProgress->Progress();

        // delete all recording's entries (e.g. settings, bookmarks, stream details)
        videoDatabase.EraseAllForPath(CPVRRecordingsPath::PATH_RECORDINGS);
      }

      videoDatabase.Close();
    }
  }

  // decrease db open refcounts; this actually closes dbs because refcounts drops to zero
  pvrDatabase->Close();
  epgDatabase->Close();

  CLog::LogFC(LOGDEBUG, LOGPVR, "{} database cleared", bResetEPGOnly ? "EPG" : "PVR and EPG");

  CLog::Log(LOGINFO, "Restarting the PVR Manager after {} database reset",
            bResetEPGOnly ? "EPG" : "PVR and EPG");
  CServiceBroker::GetPVRManager().Start();

  pDlgProgress->SetPercentage(100);
  pDlgProgress->Close();
  return true;
}
