/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowPVRRecordings.h"

#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsRecordings.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/settings/PVRSettings.h"
#include "pvr/utils/PVRPathUtils.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "video/VideoLibraryQueue.h"
#include "video/guilib/VideoPlayActionProcessor.h"
#include "video/guilib/VideoSelectActionProcessor.h"
#include "video/windows/GUIWindowVideoBase.h"

#include <memory>
#include <mutex>
#include <string>

using namespace KODI;
using namespace PVR;

namespace
{
// Numeric values are part of the Skinning API. Do not change.
constexpr unsigned int CONTROL_BTNGROUPITEMS = 5;
constexpr unsigned int CONTROL_BTNSHOWDELETED = 7;
constexpr unsigned int CONTROL_BTNSHOWMODE = 10;
constexpr unsigned int CONTROL_LABEL_HEADER1 = 29;
constexpr unsigned int CONTROL_LABEL_HEADER2 = 30;

} // unnamed namespace

CGUIWindowPVRRecordingsBase::CGUIWindowPVRRecordingsBase(bool bRadio,
                                                         int id,
                                                         const std::string& xmlFile)
  : CGUIWindowPVRBase(bRadio, id, xmlFile),
    m_settings(std::make_unique<CPVRSettings>(
        SettingsContainer({CSettings::SETTING_PVRRECORD_GROUPRECORDINGS})))
{
}

CGUIWindowPVRRecordingsBase::~CGUIWindowPVRRecordingsBase() = default;

void CGUIWindowPVRRecordingsBase::OnWindowLoaded()
{
  CONTROL_SELECT(CONTROL_BTNGROUPITEMS);
}

void CGUIWindowPVRRecordingsBase::OnDeinitWindow(int nextWindowID)
{
  if (UTILS::HasClientAndProvider(m_vecItems->GetPath()))
    m_vecItems->SetPath(""); // Open default listing next time.

  CGUIWindowPVRBase::OnDeinitWindow(nextWindowID);
}

std::string CGUIWindowPVRRecordingsBase::GetRootPath() const
{
  const CURL url{m_vecItems->GetPath()};
  std::string rootPath{CPVRRecordingsPath(m_bShowDeletedRecordings, m_bRadio)};
  rootPath += url.GetOptions();
  return rootPath;
}

std::string CGUIWindowPVRRecordingsBase::GetDirectoryPath()
{
  const std::string basePath = CPVRRecordingsPath(m_bShowDeletedRecordings, m_bRadio);
  return URIUtils::PathHasParent(m_vecItems->GetPath(), basePath) ? m_vecItems->GetPath()
                                                                  : basePath;
}

void CGUIWindowPVRRecordingsBase::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  if (pItem->IsParentFolder())
  {
    // No context menu for ".." items
    return;
  }

  bool isDeletedRecording = false;

  std::shared_ptr<CPVRRecording> recording(pItem->GetPVRRecordingInfoTag());
  if (recording)
  {
    isDeletedRecording = recording->IsDeleted();

    if (isDeletedRecording)
    {
      if (m_vecItems->GetObjectCount() > 1)
        buttons.Add(CONTEXT_BUTTON_DELETE_ALL, 19292); /* Delete all permanently */
    }
  }

  if (!isDeletedRecording)
    CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowPVRRecordingsBase::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_PARENT_DIR || action.GetID() == ACTION_NAV_BACK)
  {
    CPVRRecordingsPath path(m_vecItems->GetPath());
    if (path.IsValid() && !path.IsRecordingsRoot())
    {
      GoParentFolder();
      return true;
    }
  }
  else if (action.GetID() == ACTION_TOGGLE_WATCHED)
  {
    const std::shared_ptr<CFileItem> pItem = m_vecItems->Get(m_viewControl.GetSelectedItem());
    if (!pItem || pItem->IsParentFolder())
      return false;

    bool bUnWatched = false;
    if (pItem->HasPVRRecordingInfoTag())
      bUnWatched = pItem->GetPVRRecordingInfoTag()->GetPlayCount() == 0;
    else if (pItem->m_bIsFolder)
      bUnWatched = pItem->GetProperty("unwatchedepisodes").asInteger() > 0;
    else
      return false;

    CVideoLibraryQueue::GetInstance().MarkAsWatched(pItem, bUnWatched);
    return true;
  }

  return CGUIWindowPVRBase::OnAction(action);
}

bool CGUIWindowPVRRecordingsBase::OnPopupMenu(int iItem)
{
  if (iItem >= 0 && iItem < m_vecItems->Size())
  {
    const auto item = m_vecItems->Get(iItem);
    item->SetProperty("CheckAutoPlayNextItem", true);
  }

  return CGUIWindowPVRBase::OnPopupMenu(iItem);
}

bool CGUIWindowPVRRecordingsBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;

  return OnContextButtonDeleteAll(button) || CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowPVRRecordingsBase::Update(const std::string& strDirectory,
                                         bool updateFilterPath /* = true */)
{
  m_thumbLoader.StopThread();

  int iOldCount = m_vecItems->GetObjectCount();
  const std::string oldPath = m_vecItems->GetPath();

  bool bReturn = CGUIWindowPVRBase::Update(strDirectory);

  if (bReturn)
  {
    //! @todo does it make sense to show the non-deleted recordings, although user wants
    //!       to see the deleted recordings? Or is this just another hack to avoid misbehavior
    //!       of CGUIMediaWindow if it has no content?

    std::unique_lock lock(m_critSection);

    /* empty list for deleted recordings */
    if (m_vecItems->GetObjectCount() == 0 && m_bShowDeletedRecordings)
    {
      /* show the normal recordings instead */
      m_bShowDeletedRecordings = false;
      lock.unlock();
      Update(GetDirectoryPath());
      return bReturn;
    }
  }

  if (bReturn && iOldCount > 0 && m_vecItems->GetObjectCount() == 0 &&
      oldPath == m_vecItems->GetPath())
  {
    /* go to the parent folder if we're in a subdirectory and for instance just deleted the last item */
    const CPVRRecordingsPath path(m_vecItems->GetPath());
    if (path.IsValid() && !path.IsRecordingsRoot())
      GoParentFolder();
  }
  return bReturn;
}

void CGUIWindowPVRRecordingsBase::UpdateButtons()
{
  int iWatchMode = CMediaSettings::GetInstance().GetWatchedMode("recordings");
  int iStringId = 257; // "Error"

  if (iWatchMode == WatchedModeAll)
    iStringId = 22015; // "All recordings"
  else if (iWatchMode == WatchedModeUnwatched)
    iStringId = 16101; // "Unwatched"
  else if (iWatchMode == WatchedModeWatched)
    iStringId = 16102; // "Watched"

  SET_CONTROL_LABEL(CONTROL_BTNSHOWMODE, g_localizeStrings.Get(iStringId));

  const bool bGroupRecordings{
      m_settings->GetBoolValue(CSettings::SETTING_PVRRECORD_GROUPRECORDINGS)};
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTNGROUPITEMS, bGroupRecordings);

  auto* btnShowDeleted{static_cast<CGUIRadioButtonControl*>(GetControl(CONTROL_BTNSHOWDELETED))};
  if (btnShowDeleted)
  {
    btnShowDeleted->SetVisible(
        m_bRadio ? CServiceBroker::GetPVRManager().Recordings()->HasDeletedRadioRecordings()
                 : CServiceBroker::GetPVRManager().Recordings()->HasDeletedTVRecordings());
    btnShowDeleted->SetSelected(m_bShowDeletedRecordings);
  }

  CGUIWindowPVRBase::UpdateButtons();

  // If we are filtering by client id / provider id, expose provider's name.
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, UTILS::GetProviderNameFromPath(m_vecItems->GetPath()));

  const CPVRRecordingsPath path(m_vecItems->GetPath());
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER2,
                    bGroupRecordings && path.IsValid() ? path.GetUnescapedDirectoryPath() : "");
}

namespace
{
class CVideoSelectActionProcessor : public VIDEO::GUILIB::CVideoSelectActionProcessor
{
public:
  CVideoSelectActionProcessor(CGUIWindowPVRRecordingsBase& window,
                              const std::shared_ptr<CFileItem>& item,
                              int itemIndex)
    : VIDEO::GUILIB::CVideoSelectActionProcessor(item), m_window(window), m_itemIndex(itemIndex)
  {
  }

protected:
  bool OnChooseSelected() override
  {
    m_window.OnPopupMenu(m_itemIndex);
    return true;
  }

private:
  CGUIWindowPVRRecordingsBase& m_window;
  const int m_itemIndex{-1};
};
} // namespace

bool CGUIWindowPVRRecordingsBase::OnMessage(CGUIMessage& message)
{
  bool bReturn = false;
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
      if (message.GetSenderId() == m_viewControl.GetCurrentControl())
      {
        int iItem = m_viewControl.GetSelectedItem();
        if (iItem >= 0 && iItem < m_vecItems->Size())
        {
          const CFileItemPtr item(m_vecItems->Get(iItem));
          switch (message.GetParam1())
          {
            case ACTION_SELECT_ITEM:
            case ACTION_MOUSE_LEFT_CLICK:
            case ACTION_PLAYER_PLAY:
            {
              const CPVRRecordingsPath path(m_vecItems->GetPath());
              if (path.IsValid() && path.IsRecordingsRoot() && item->IsParentFolder())
              {
                // handle .. item, which is only visible if list of recordings is empty.
                CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_HOME);
                bReturn = true;
                break;
              }

              if (!item->IsParentFolder() && message.GetParam1() == ACTION_PLAYER_PLAY)
              {
                KODI::VIDEO::GUILIB::CVideoPlayActionProcessor proc{item};
                bReturn = proc.ProcessDefaultAction();
              }
              else if (item->m_bIsFolder)
              {
                // recording folders and ".." folders in subfolders are handled by base class.
                bReturn = false;
              }
              else
              {
                CVideoSelectActionProcessor proc(*this, item, iItem);
                bReturn = proc.ProcessDefaultAction();
              }
              break;
            }
            case ACTION_CONTEXT_MENU:
            case ACTION_MOUSE_RIGHT_CLICK:
              OnPopupMenu(iItem);
              bReturn = true;
              break;
            case ACTION_SHOW_INFO:
              CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().ShowRecordingInfo(*item);
              bReturn = true;
              break;
            case ACTION_DELETE_ITEM:
              CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().DeleteRecording(*item);
              bReturn = true;
              break;
            default:
              bReturn = false;
              break;
          }
        }
      }
      else if (message.GetSenderId() == CONTROL_BTNGROUPITEMS)
      {
        const std::shared_ptr<CSettings> settings =
            CServiceBroker::GetSettingsComponent()->GetSettings();
        settings->ToggleBool(CSettings::SETTING_PVRRECORD_GROUPRECORDINGS);
        settings->Save();
        Refresh(true);
      }
      else if (message.GetSenderId() == CONTROL_BTNSHOWDELETED)
      {
        const auto* radioButton{
            static_cast<CGUIRadioButtonControl*>(GetControl(CONTROL_BTNSHOWDELETED))};
        if (radioButton)
        {
          m_bShowDeletedRecordings = radioButton->IsSelected();
          Update(GetDirectoryPath());
        }
        bReturn = true;
      }
      else if (message.GetSenderId() == CONTROL_BTNSHOWMODE)
      {
        CMediaSettings::GetInstance().CycleWatchedMode("recordings");
        CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
        OnFilterItems(GetProperty("filter").asString());
        UpdateButtons();
        return true;
      }
      break;
    case GUI_MSG_REFRESH_LIST:
    {
      switch (static_cast<PVREvent>(message.GetParam1()))
      {
        using enum PVREvent;

        case CurrentItem:
        case Epg:
        case EpgActiveItem:
        case EpgContainer:
        case Timers:
          SetInvalid();
          break;

        case RecordingsInvalidated:
        case TimersInvalidated:
          Refresh(true);
          break;

        default:
          break;
      }
      break;
    }
    default:
      break;
  }

  return bReturn || CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRRecordingsBase::OnContextButtonDeleteAll(CONTEXT_BUTTON button) const
{
  if (button == CONTEXT_BUTTON_DELETE_ALL)
  {
    CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().DeleteAllRecordingsFromTrash();
    return true;
  }

  return false;
}

void CGUIWindowPVRRecordingsBase::OnPrepareFileItems(CFileItemList& items)
{
  if (items.IsEmpty())
    return;

  CFileItemList files;
  for (const auto& item : items)
  {
    if (!item->m_bIsFolder)
      files.Add(item);
  }

  if (!files.IsEmpty())
  {
    if (m_database.Open())
    {
      CGUIWindowVideoBase::LoadVideoInfo(files, m_database, false);
      m_database.Close();
    }
    m_thumbLoader.Load(files);
  }

  CGUIWindowPVRBase::OnPrepareFileItems(items);
}

bool CGUIWindowPVRRecordingsBase::GetFilteredItems(const std::string& filter, CFileItemList& items)
{
  bool listchanged = CGUIWindowPVRBase::GetFilteredItems(filter, items);

  int watchMode = CMediaSettings::GetInstance().GetWatchedMode("recordings");

  CFileItemPtr item;
  for (int i = 0; i < items.Size(); i++)
  {
    item = items.Get(i);

    if (item->IsParentFolder()) // Don't delete the go to parent folder
      continue;

    if (!item->HasPVRRecordingInfoTag())
      continue;

    int iPlayCount = item->GetPVRRecordingInfoTag()->GetPlayCount();
    if ((watchMode == WatchedModeWatched && iPlayCount == 0) ||
        (watchMode == WatchedModeUnwatched && iPlayCount > 0))
    {
      items.Remove(i);
      i--;
      listchanged = true;
    }
  }

  // Remove the parent folder item, if it's the only item in the folder.
  if (items.GetObjectCount() == 0 && items.GetFileCount() > 0 && items.Get(0)->IsParentFolder())
    items.Remove(0);

  return listchanged;
}
