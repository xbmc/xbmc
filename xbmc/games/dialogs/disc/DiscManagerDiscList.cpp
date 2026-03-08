/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscManagerDiscList.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "games/GameUtils.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "games/dialogs/disc/DialogGameDiscManager.h"
#include "guilib/GUIListItem.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <assert.h>
#include <optional>

using namespace KODI;
using namespace GAME;

CDiscManagerDiscList::CDiscManagerDiscList(GameClientPtr gameClient,
                                           CDialogGameDiscManager& discManager,
                                           int parentID)
  : IListProvider(parentID),
    m_gameClient(std::move(gameClient)),
    m_discManager(discManager)
{
  assert(m_gameClient.get() != nullptr);
}

std::unique_ptr<IListProvider> CDiscManagerDiscList::Clone()
{
  return std::make_unique<CDiscManagerDiscList>(*this);
}

bool CDiscManagerDiscList::Update(bool forceRefresh)
{
  if (m_items.empty())
    UpdateItems();

  bool dirty{false};
  std::swap(dirty, m_dirty);
  return dirty;
}

void CDiscManagerDiscList::Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items)
{
  items.clear();
  for (auto item : m_items)
    items.emplace_back(std::static_pointer_cast<CGUIListItem>(item));
}

void CDiscManagerDiscList::Reset()
{
  UpdateItems();
}

bool CDiscManagerDiscList::OnClick(const std::shared_ptr<CGUIListItem>& item)
{
  auto fileItem = std::dynamic_pointer_cast<CFileItem>(item);
  if (fileItem)
  {
    const size_t discIndex = static_cast<size_t>(fileItem->GetProperty("discIndex").asInteger());
    const bool isNoDisc = fileItem->GetProperty("isNoDisc").asBoolean();
    m_discManager.OnDiscSelect(discIndex, isNoDisc);
    return true;
  }

  return false;
}

void CDiscManagerDiscList::UpdateItems()
{
  m_items.clear();

  const CGameClientDiscModel& discList = m_gameClient->Discs().GetDiscs();

  const std::optional<size_t> selectedDiscIndex = discList.GetSelectedDiscIndex();

  for (size_t i = 0; i < discList.Size(); ++i)
  {
    // If disc has been removed from the list, this could be a zombie entry
    if (discList.IsRemovedSlotByIndex(i))
      continue;

    const std::string path = discList.GetPathByIndex(i);
    const std::string label = discList.GetLabelByIndex(i);

    auto item = std::make_shared<CFileItem>(label);
    item->SetPath(path);
    item->Select(selectedDiscIndex.has_value() && *selectedDiscIndex == i);
    item->SetProperty("discIndex", static_cast<int64_t>(i));
    item->SetProperty("isNoDisc", false);
    item->SetProperty("isplaying", false);

    m_items.emplace_back(std::move(item));
  }

  if (m_discManager.AllowSelectNoDisc() || m_items.empty())
  {
    auto noDiscItem = std::make_shared<CFileItem>("No disc");
    noDiscItem->SetPath("");
    noDiscItem->Select(discList.IsSelectedNoDisc());
    noDiscItem->SetProperty("discIndex", static_cast<int64_t>(discList.Size()));
    noDiscItem->SetProperty("isNoDisc", true);
    noDiscItem->SetProperty("isplaying", false);

    m_items.emplace_back(std::move(noDiscItem));
  }

  m_dirty = true;
}
