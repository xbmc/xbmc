/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameCheats.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "games/GameUtils.h"
#include "games/addons/GameClient.h"
#include "games/addons/cheats/GameClientCheats.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIMacros.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "jobs/JobManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace KODI::GAME;

namespace
{
constexpr int HEADING_GET_MORE = 21452; // "Get more..."
constexpr int HEADING_CHEATS = 35320; // "Cheats"
constexpr int HEADING_ENABLED = 305; // "Enabled"
constexpr int HEADING_NO_CHEATS = 35323; // "No cheats found for this game"

constexpr int CONTROL_HEADING = 1083900;
constexpr int CONTROL_CHEATS_LIST = 1083901;
constexpr int CONTROL_ENABLED_SUMMARY = 1083902;
constexpr int CONTROL_BUTTON_TEMPLATE = 1083903;
constexpr int CONTROL_RADIO_TEMPLATE = 1083904;
constexpr int CONTROL_CLOSE = 1083905;
constexpr int CONTROL_SCROLLBAR = 1083906;
constexpr int CONTROL_ENABLED_HEADING = 1083907;

// Keep generated rows above the skin controls and away from navigation ID 0.
constexpr int CONTROL_CHEATS_START = 1083909;
} // namespace

CDialogGameCheats::CDialogGameCheats()
  : CGUIDialog(WINDOW_DIALOG_GAME_CHEATS, "DialogGameControllers.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CDialogGameCheats::~CDialogGameCheats() = default;

bool CDialogGameCheats::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_SETFOCUS:
    {
      if (message.GetControlId() == CONTROL_CHEATS_LIST && m_restoreListFocus)
      {
        const auto* list = dynamic_cast<CGUIControlGroupList*>(GetControl(CONTROL_CHEATS_LIST));
        const CGUIControl* control = list ? GetControl(list->GetFocusedControlID()) : nullptr;
        if (control && control->CanFocus())
        {
          // The group may still be scrolling the remembered row into view.
          SET_CONTROL_FOCUS(control->GetID(), 0);
          return true;
        }
      }
      break;
    }
    case GUI_MSG_FOCUSED:
    {
      const bool handled = CGUIDialog::OnMessage(message);
      const int focusedControl = GetFocusedControlID();
      if (IsListAction(focusedControl))
        m_restoreListFocus = true;
      return handled;
    }
    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetControlId() == CONTROL_SCROLLBAR && message.GetParam1() == GUI_MSG_PAGE_CHANGE)
        m_restoreListFocus = false;
      break;
    }
    case GUI_MSG_CLICKED:
    {
      const int controlId = message.GetSenderId();
      if (controlId == CONTROL_CLOSE)
      {
        Close();
        return true;
      }
      if (IsCheatControl(controlId))
      {
        ToggleCheat(controlId);
        return true;
      }
      if (m_getMoreControl != 0 && controlId == m_getMoreControl)
      {
        GetMore();
        return true;
      }
      break;
    }
    case GUI_MSG_UPDATE:
    {
      if (message.GetSenderId() == WINDOW_DIALOG_GAME_CHEATS)
      {
        if (!IsDialogRunning() || m_closing)
          return true;

        const int focusedControl = GetFocusedControlID();
        const bool listFocused = IsListAction(focusedControl);
        auto* list = dynamic_cast<CGUIControlGroupList*>(GetControl(CONTROL_CHEATS_LIST));
        const float scrollOffset = list ? list->GetScrollOffset() : 0;
        const int focusedIndex = list ? list->GetFocusedControlID() - CONTROL_CHEATS_START : -1;
        const bool restoreListFocus = m_restoreListFocus;
        InitializeControls();

        const int rowCount = static_cast<int>(m_cheats.size()) + (m_getMoreControl != 0 ? 1 : 0);
        const int selectedControl =
            focusedIndex >= 0 && rowCount > 0
                ? CONTROL_CHEATS_START + std::min(focusedIndex, rowCount - 1)
                : 0;
        m_restoreListFocus = restoreListFocus && selectedControl != 0;
        if (list)
        {
          list->SetScrollOffset(scrollOffset);
          CGUIMessage select(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_CHEATS_LIST,
                             m_restoreListFocus ? selectedControl : 0);
          list->OnMessage(select);
        }

        int restoreControl = listFocused ? CONTROL_CHEATS_LIST : focusedControl;
        if (restoreControl == CONTROL_SCROLLBAR && rowCount == 0)
          restoreControl = CONTROL_CLOSE;
        const CGUIControl* control = GetControl(restoreControl);
        if (!control || !control->CanFocus())
          restoreControl = list && list->CanFocus() ? CONTROL_CHEATS_LIST : CONTROL_CLOSE;
        SET_CONTROL_FOCUS(restoreControl, 0);
        return true;
      }
      break;
    }
    default:
      break;
  }
  return CGUIDialog::OnMessage(message);
}

void CDialogGameCheats::OnInitWindow()
{
  ResetControlStates();
  InitializeControls();
  CGUIDialog::OnInitWindow();
}

void CDialogGameCheats::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
  ClearControls();
}

void CDialogGameCheats::InitializeControls()
{
  const GameClientPtr gameClient = CGameUtils::GetPlayingGameClient();
  CreateControls(gameClient ? gameClient->Cheats().GetCheats() : std::vector<Cheat>{},
                 gameClient && gameClient->Cheats().CanInstallCheats());
}

void CDialogGameCheats::CreateControls(std::vector<Cheat> cheats, bool getMore)
{
  ClearControls();
  m_cheats = std::move(cheats);
  m_defaultControl = CONTROL_CLOSE;

  SET_CONTROL_LABEL(CONTROL_HEADING, HEADING_CHEATS);
  SET_CONTROL_LABEL(CONTROL_CLOSE, 15067); // "Close"
  SET_CONTROL_HIDDEN(CONTROL_BUTTON_TEMPLATE);
  SET_CONTROL_HIDDEN(CONTROL_RADIO_TEMPLATE);

  auto* list = dynamic_cast<CGUIControlGroupList*>(GetControl(CONTROL_CHEATS_LIST));
  auto* radio = dynamic_cast<CGUIRadioButtonControl*>(GetControl(CONTROL_RADIO_TEMPLATE));
  auto* button = dynamic_cast<CGUIButtonControl*>(GetControl(CONTROL_BUTTON_TEMPLATE));
  if (list)
  {
    const auto addControl = [this, list](CGUIControl* control, int id)
    {
      control->SetID(id);
      control->SetVisible(true);
      control->SetWidth(list->GetWidth());
      control->AllocResources();
      list->AddControl(control);
      if (m_defaultControl == CONTROL_CLOSE && control->CanFocus())
        m_defaultControl = id;
    };

    if (radio)
    {
      for (size_t index = 0; index < m_cheats.size(); ++index)
      {
        const Cheat& cheat = m_cheats[index];
        auto* control = radio->Clone();
        control->SetLabel(!cheat.description.empty() ? cheat.description : cheat.code);
        control->SetSelected(cheat.enabled);
        addControl(control, CONTROL_CHEATS_START + static_cast<int>(index));
      }
    }
    if (getMore && button)
    {
      m_getMoreControl = CONTROL_CHEATS_START + static_cast<int>(m_cheats.size());
      auto* control = button->Clone();
      control->SetLabel(
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(HEADING_GET_MORE));
      addControl(control, m_getMoreControl);
    }
    list->SetInvalid();
    CGUIMessage reset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SCROLLBAR,
                      static_cast<int>(list->Size()), static_cast<int>(list->GetTotalSize()));
    OnMessage(reset);
  }
  UpdateEnabledSummary();
}

void CDialogGameCheats::ClearControls()
{
  if (auto* list = dynamic_cast<CGUIControlGroupList*>(GetControl(CONTROL_CHEATS_LIST)))
  {
    list->FreeResources();
    list->ClearAll();
  }
  m_cheats.clear();
  m_getMoreControl = 0;
  m_restoreListFocus = false;
}

bool CDialogGameCheats::IsCheatControl(int controlId) const
{
  return controlId >= CONTROL_CHEATS_START &&
         static_cast<size_t>(controlId - CONTROL_CHEATS_START) < m_cheats.size();
}

bool CDialogGameCheats::IsListAction(int controlId) const
{
  return IsCheatControl(controlId) || (m_getMoreControl != 0 && controlId == m_getMoreControl);
}

void CDialogGameCheats::ToggleCheat(int controlId)
{
  const GameClientPtr gameClient = CGameUtils::GetPlayingGameClient();
  if (!gameClient)
    return;

  const auto index = static_cast<unsigned int>(controlId - CONTROL_CHEATS_START);
  Cheat& cheat = m_cheats[index];
  if (!gameClient->Cheats().SetEnabled(index, !cheat.enabled, cheat))
  {
    CGUIMessage refresh(GUI_MSG_UPDATE, GetID(), -1);
    OnMessage(refresh);
    return;
  }
  cheat.enabled = !cheat.enabled;
  if (auto* control = dynamic_cast<CGUIRadioButtonControl*>(GetControl(controlId)))
    control->SetSelected(cheat.enabled);
  UpdateEnabledSummary();
}

std::string CDialogGameCheats::EnabledSummary() const
{
  std::vector<std::string> names;
  for (const Cheat& cheat : m_cheats)
  {
    if (cheat.enabled)
      names.emplace_back(!cheat.description.empty() ? cheat.description : cheat.code);
  }
  return StringUtils::Join(names, "[CR]");
}

void CDialogGameCheats::UpdateEnabledSummary()
{
  const std::string summary = EnabledSummary();
  SET_CONTROL_LABEL(
      CONTROL_ENABLED_HEADING,
      summary.empty()
          ? ""
          : CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(HEADING_ENABLED));

  SET_CONTROL_LABEL(CONTROL_ENABLED_SUMMARY, summary);
}

void CDialogGameCheats::GetMore()
{
  const std::shared_ptr<CJobManager> jobManager = CServiceBroker::GetJobManager();
  if (!jobManager)
    return;

  const GameClientPtr gameClient = CGameUtils::GetPlayingGameClient();
  auto install = gameClient ? gameClient->Cheats().GetInstallTask()
                            : std::function<CGameClientCheats::InstallResult()>{};
  if (!install)
    return;

  jobManager->Submit(
      [install = std::move(install)]()
      {
        if (install() == CGameClientCheats::InstallResult::NO_CHEATS)
        {
          CGUIDialogKaiToast::QueueNotification(
              CGUIDialogKaiToast::Info,
              CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(HEADING_CHEATS),
              CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(HEADING_NO_CHEATS));
        }
      });
}
