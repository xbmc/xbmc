/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscManagerButtons.h"

#include "ServiceBroker.h"
#include "games/dialogs/disc/DialogGameDiscManager.h"
#include "games/dialogs/disc/DiscManagerActions.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIMessageIDs.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"

using namespace KODI;
using namespace GAME;

namespace
{
constexpr int CONTROL_BUTTON_SELECT_DISC = 108323;
constexpr int CONTROL_BUTTON_EJECT_INSERT = 108324;
constexpr int CONTROL_BUTTON_ADD = 108325;
constexpr int CONTROL_BUTTON_REMOVE = 108326;
constexpr int CONTROL_BUTTON_APPLY_DISC_CHANGE = 108327;
constexpr int CONTROL_BUTTON_RESUME_GAME = 108328;
} // namespace

CDiscManagerButtons::CDiscManagerButtons(CDialogGameDiscManager& discManager,
                                         CDiscManagerActions& discActions)
  : m_discManager(discManager),
    m_discActions(discActions)
{
}

bool CDiscManagerButtons::OnClick(int controlId)
{
  switch (controlId)
  {
    case CONTROL_BUTTON_SELECT_DISC:
    {
      m_discActions.OnSelectDisc();
      return true;
    }
    case CONTROL_BUTTON_EJECT_INSERT:
    {
      m_discActions.OnEjectInsert();
      return true;
    }
    case CONTROL_BUTTON_ADD:
    {
      m_discActions.OnAdd();
      return true;
    }
    case CONTROL_BUTTON_REMOVE:
    {
      m_discActions.OnRemove();
      return true;
    }
    case CONTROL_BUTTON_APPLY_DISC_CHANGE:
    {
      m_discActions.OnApplyDiscChange();
      return true;
    }
    case CONTROL_BUTTON_RESUME_GAME:
    {
      m_discActions.OnResumeGame();
      return true;
    }
    default:
      break;
  }

  return false;
}

void CDiscManagerButtons::UpdateButtons(bool ejected, const std::string& selectedDisc)
{
  auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  // Update "Select disc" label2
  CGUIMessage msgSelectDiskLabel2(GUI_MSG_LABEL2_SET, m_discManager.GetID(),
                                  CONTROL_BUTTON_SELECT_DISC, 0);
  msgSelectDiskLabel2.SetLabel(selectedDisc);
  m_discManager.OnMessage(msgSelectDiskLabel2);

  // Update "Eject" / "Insert" label
  CGUIMessage msgEjectInsertLabel(GUI_MSG_LABEL_SET, m_discManager.GetID(),
                                  CONTROL_BUTTON_EJECT_INSERT, 0);
  if (ejected)
    msgEjectInsertLabel.SetLabel(strings.Get(35276)); // "Insert"
  else
    msgEjectInsertLabel.SetLabel(strings.Get(35275)); // "Eject"
  m_discManager.OnMessage(msgEjectInsertLabel);

  // Update "Eject" / "Insert" label2
  CGUIMessage msgEjectInsertLabel2(GUI_MSG_LABEL2_SET, m_discManager.GetID(),
                                   CONTROL_BUTTON_EJECT_INSERT, 0);
  if (ejected)
    msgEjectInsertLabel2.SetLabel(strings.Get(162)); // "Tray open"
  else
    msgEjectInsertLabel2.SetLabel("");
  m_discManager.OnMessage(msgEjectInsertLabel2);
}

void CDiscManagerButtons::SetFocus()
{
  // Focus "Select disc" button
  CGUIMessage msgSelectFirst(GUI_MSG_SETFOCUS, m_discManager.GetID(), CONTROL_BUTTON_SELECT_DISC,
                             0);
  m_discManager.OnMessage(msgSelectFirst);
}
