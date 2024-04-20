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

protected:
  // implementation of CDialogGameVideoSelect
  std::string GetHeading() override;
  void PreInit() override;
  void GetItems(CFileItemList& items) override;
  void OnItemFocus(unsigned int index) override;
  unsigned int GetFocusedItem() const override;
  void PostExit() override;
  bool OnClickAction() override;

private:
  void InitVideoFilters();

  static void GetProperties(const CFileItem& item,
                            std::string& videoFilter,
                            std::string& description);

  CFileItemList m_items;

  //! \brief Set to true when a description has first been set
  bool m_bHasDescription = false;
};
} // namespace GAME
} // namespace KODI
