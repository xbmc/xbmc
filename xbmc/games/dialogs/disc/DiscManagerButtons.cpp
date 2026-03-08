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
#include "games/dialogs/disc/DiscManagerIDs.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIMessageIDs.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"

using namespace KODI;
using namespace GAME;

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
    case CONTROL_BUTTON_DELETE:
    {
      m_discActions.OnDelete();
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

  // Update "Disc selection" label2
  CGUIMessage msgSelectDiskLabel2(GUI_MSG_LABEL2_SET, m_discManager.GetID(),
                                  CONTROL_BUTTON_SELECT_DISC);
  msgSelectDiskLabel2.SetLabel(selectedDisc);
  m_discManager.OnMessage(msgSelectDiskLabel2);

  // Update "Eject" / "Insert" label
  CGUIMessage msgEjectInsertLabel(GUI_MSG_LABEL_SET, m_discManager.GetID(),
                                  CONTROL_BUTTON_EJECT_INSERT);
  if (ejected)
    msgEjectInsertLabel.SetLabel(strings.Get(35276)); // "Insert"
  else
    msgEjectInsertLabel.SetLabel(strings.Get(35275)); // "Eject"
  m_discManager.OnMessage(msgEjectInsertLabel);

  // Update "Eject" / "Insert" label2
  CGUIMessage msgEjectInsertLabel2(GUI_MSG_LABEL2_SET, m_discManager.GetID(),
                                   CONTROL_BUTTON_EJECT_INSERT);
  if (ejected)
    msgEjectInsertLabel2.SetLabel(strings.Get(162)); // "Tray open"
  else
    msgEjectInsertLabel2.SetLabel("");
  m_discManager.OnMessage(msgEjectInsertLabel2);
}

void CDiscManagerButtons::SetFocus(unsigned int menuItemIndex)
{
  if (menuItemIndex >= MENU_ITEM_COUNT)
    return;

  // Calculate control offset
  const int controlId = CONTROL_BUTTON_SELECT_DISC + menuItemIndex;

  CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS, m_discManager.GetID(), controlId);
  m_discManager.OnMessage(msgSetFocus);
}
