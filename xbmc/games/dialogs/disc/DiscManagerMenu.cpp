/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscManagerMenu.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "games/dialogs/disc/DiscManagerActions.h"
#include "games/dialogs/disc/DiscManagerGame.h"
#include "games/dialogs/disc/DiscManagerIDs.h"
#include "guilib/GUIListItem.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

CDiscManagerMenu::CDiscManagerMenu(CDiscManagerActions& discActions,
                                   CDiscManagerGame& discGame,
                                   int parentID)
  : IListProvider(parentID),
    m_discActions(discActions),
    m_discGame(discGame)
{
}

std::unique_ptr<IListProvider> CDiscManagerMenu::Clone()
{
  return std::make_unique<CDiscManagerMenu>(*this);
}

bool CDiscManagerMenu::Update(bool forceRefresh)
{
  const bool ejected = m_discGame.IsEjected();

  // Always update when eject state changes
  if (ejected != m_ejected || forceRefresh)
  {
    UpdateItems();
    m_ejected = ejected;
    return true;
  }

  return false;
}

void CDiscManagerMenu::Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items)
{
  items = m_items;
}

bool CDiscManagerMenu::OnClick(const std::shared_ptr<CGUIListItem>& item)
{
  // Ensure we have a full menu
  if (m_items.size() < MENU_ITEM_COUNT)
    return false;

  if (item == m_items[MENU_INDEX_SELECT_DISC])
  {
    m_discActions.OnSelectDisc();
    return true;
  }
  else if (item == m_items[MENU_INDEX_EJECT_INSERT])
  {
    m_discActions.OnEjectInsert();
    return true;
  }
  else if (item == m_items[MENU_INDEX_ADD_DISC])
  {
    m_discActions.OnAdd();
    return true;
  }
  else if (item == m_items[MENU_INDEX_DELETE_DISC])
  {
    m_discActions.OnDelete();
    return true;
  }
  else if (item == m_items[MENU_INDEX_RESUME_GAME])
  {
    m_discActions.OnResumeGame();
    return true;
  }

  return false;
}

void CDiscManagerMenu::OnReplace(IListProvider& previousProvider)
{
  m_items.clear();

  previousProvider.Fetch(m_items);

  // Inform the skin developer if we don't have a complete menu
  if (m_items.size() < MENU_ITEM_COUNT)
  {
    CLog::Log(LOGERROR, "Disc Manager menu has only {} items. Expected {}.", m_items.size(),
              MENU_ITEM_COUNT);
  }
  else if (m_items.size() > MENU_ITEM_COUNT)
  {
    CLog::Log(LOGINFO, "Disc Manager menu has {} items. Expected {}. Extra items will be ignored.",
              m_items.size(), MENU_ITEM_COUNT);
  }

  UpdateItems();
}

void CDiscManagerMenu::UpdateItems()
{
  while (m_items.size() < MENU_ITEM_COUNT)
    m_items.emplace_back(std::make_shared<CFileItem>());

  bool ejected;
  std::string selectedDisc;
  m_discGame.GetState(ejected, selectedDisc);

  m_items[MENU_INDEX_SELECT_DISC]->SetLabel2(selectedDisc);

  // Set eject/insert item labels
  UpdateEjectButton(ejected);
}

void CDiscManagerMenu::UpdateEjectButton(bool ejected)
{
  auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  if (ejected)
  {
    m_items[MENU_INDEX_EJECT_INSERT]->SetLabel(strings.Get(35276)); // "Insert"
    m_items[MENU_INDEX_EJECT_INSERT]->SetLabel2(strings.Get(162)); // "Tray open"
  }
  else
  {
    m_items[MENU_INDEX_EJECT_INSERT]->SetLabel(strings.Get(35275)); // "Eject"
    m_items[MENU_INDEX_EJECT_INSERT]->SetLabel2("");
  }
}
