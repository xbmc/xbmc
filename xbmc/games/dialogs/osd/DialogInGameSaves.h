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
#include "FileItemList.h"
#include "guilib/GUIListItem.h"

#include <string>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CDialogInGameSaves : public CDialogGameVideoSelect
{
public:
  CDialogInGameSaves();
  ~CDialogInGameSaves() override = default;

  // implementation of CGUIControl via CDialogGameVideoSelect
  bool OnMessage(CGUIMessage& message) override;

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
  void OnItemRefresh(const std::string& itemPath, const std::shared_ptr<CGUIListItem>& itemInfo);

  /*!
   * \brief Translates the GUI list item received in a GUI message into a
   *        CFileItem with savestate properties
   *
   * When a savestate is overwritten, we optimistically populate the GUI list
   * with a simulated savestate for immediate user feedback. Later (about a
   * quarter second) a message arrives with the real savestate info.
   *
   * \param messagePath The savestate path, pass as the message's string param
   * \param messageItem The savestate info, if known, or empty if unknown
   *
   * If messageItem is empty, the savestate will be loaded from disk, which
   * is potentially expensive.
   *
   * \return A savestate item for the GUI, or empty if no savestate information
   *         can be obtained
   */
  static CFileItemPtr TranslateMessageItem(const std::string& messagePath,
                                           const std::shared_ptr<CGUIListItem>& messageItem);

  CFileItemList m_savestateItems;
  const CFileItemPtr m_newSaveItem;
  unsigned int m_focusedItemIndex = false;
};
} // namespace GAME
} // namespace KODI
