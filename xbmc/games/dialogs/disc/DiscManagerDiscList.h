/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "guilib/listproviders/IListProvider.h"

#include <memory>
#include <vector>

class CFileItem;

namespace KODI
{
namespace GAME
{
class CDialogGameDiscManager;

/*!
 * \ingroup games
 */
class CDiscManagerDiscList : public IListProvider
{
public:
  explicit CDiscManagerDiscList(GameClientPtr gameClient,
                                CDialogGameDiscManager& discManager,
                                int parentID);
  explicit CDiscManagerDiscList(const CDiscManagerDiscList& other) = default;
  ~CDiscManagerDiscList() override = default;

  // Implementation of IListProvider
  std::unique_ptr<IListProvider> Clone() override;
  bool Update(bool forceRefresh) override;
  void Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items) override;
  void Reset() override;
  bool OnClick(const std::shared_ptr<CGUIListItem>& item) override;

private:
  // Dialog interface
  void UpdateItems();

  // Construction parameters
  const GameClientPtr m_gameClient;
  CDialogGameDiscManager& m_discManager;

  // GUI parameters
  std::vector<std::shared_ptr<CFileItem>> m_items;
  bool m_dirty{false};
};
} // namespace GAME
} // namespace KODI
