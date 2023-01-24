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

#include <string>

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
  bool OnClickAction() override;
  bool OnMenuAction() override;
  bool OnOverwriteAction() override;
  bool OnRenameAction() override;
  bool OnDeleteAction() override;

  void OnNewSave();
  void OnLoad(CFileItem& focusedItem);
  void OnOverwrite(CFileItem& focusedItem);
  void OnRename(CFileItem& focusedItem);
  void OnDelete(CFileItem& focusedItem);

private:
  void InitSavedGames();

  CFileItemList m_savestateItems;
  const CFileItemPtr m_newSaveItem;
  unsigned int m_focusedItemIndex = false;
};
} // namespace GAME
} // namespace KODI
