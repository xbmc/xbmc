/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DialogGameVideoSelect.h"
#include "FileItem.h"

namespace KODI
{
namespace GAME
{
class CDialogInGameSaves : public CDialogGameVideoSelect
{
public:
  CDialogInGameSaves();
  ~CDialogInGameSaves() override = default;

protected:
  // implementation of CDialogGameVideoSelect
  std::string GetHeading() override;
  void PreInit() override;
  void GetItems(CFileItemList& items) override;
  void OnItemFocus(unsigned int index) override;
  unsigned int GetFocusedItem() const override;
  void PostExit() override;
  void OnClickAction() override;

private:
  void InitSavedGames();

  static void GetProperties(const CFileItem& item,
                            std::string& videoFilter,
                            std::string& description);

  CFileItemList m_items;
  unsigned int m_focusedItemIndex = false;
};
} // namespace GAME
} // namespace KODI
