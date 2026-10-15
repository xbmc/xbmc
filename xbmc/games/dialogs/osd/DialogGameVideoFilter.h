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
};
} // namespace GAME
} // namespace KODI
