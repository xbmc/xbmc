/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DialogGameVideoSelect.h"
#include "FileItem.h"
#include "FileItemList.h"

#include <chrono>
#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CDialogGameVideoFilter : public CDialogGameVideoSelect
{
public:
  CDialogGameVideoFilter();
  ~CDialogGameVideoFilter() override = default;

  // implementation of CGUIControl via CDialogGameVideoSelect
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;

  // implementation of CGUIWindow via CDialogGameVideoSelect
  void FrameMove() override;

protected:
  // implementation of CGUIWindow via CDialogGameVideoSelect
  void OnInitWindow() override;

  // implementation of CDialogGameVideoSelect
  std::string GetHeading() override;
  void PreInit() override;
  void GetItems(CFileItemList& items) override;
  void OnItemFocus(unsigned int index) override;
  unsigned int GetFocusedItem() const override;
  void PostExit() override;
  bool OnClickAction() override;
  void RefreshList() override;

private:
  void InitScalingMethods();
  void InitVideoFilters();
  void InitGetMoreButton();
  void InitCategories();
  void BindCategories();
  bool HasCategories();

  /*!
   * \brief Show the filters of the category the category list has selected
   */
  void UpdateCategory();

  /*!
   * \brief Move from the last filter to the next category's first, or from the
   * first filter to the previous category's last
   *
   * \return True if the focused filter was at that end of its row
   */
  bool MoveAcrossCategories(bool forward);

  /*!
   * \brief Tell the skin which way the filters moved to another category, so
   * it can slide the new ones in from that side
   */
  void AnnounceCategoryMove(bool forward);
  void ApplyVideoFilter(const CFileItem& item);
  void OnGetMore();
  void OnGetMoreComplete();

  CFileItemList m_items;
  std::vector<CFileItemPtr> m_shownItems;
  std::vector<std::string> m_categories;

  static std::string GetLocalizedString(uint32_t code);

  struct VideoFilterProperties
  {
    std::string path;
    std::string name;
    std::string category;
  };

  // GUI state
  unsigned int m_focusedItemIndex{0};
  unsigned int m_categoryIndex{0};
  bool m_regenerateList{false};
  bool m_refreshingList{false};
  bool m_choosingCategory{false};
  std::string m_categoryMove;
  std::chrono::steady_clock::time_point m_categoryMoveStart;
  // Frames until a move made during the last one's slide is told to the skin
  unsigned int m_categoryMoveDelay{0};
};
} // namespace GAME
} // namespace KODI
