/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CFileItem;

namespace UTILS
{
namespace GUILIB
{
class CGUIContentUtils
{
public:
  /*! \brief Check whether an information dialog is available for the given item.
    \param item The item to process
    \return True if an information dialog is available, false otherwise
  */
  static bool HasInfoForItem(const CFileItem& item);

  /*! \brief Show an information dialog for the given item.
    \param item The item to process
    \return True if an information dialog was displayed, false otherwise
  */
  static bool ShowInfoForItem(const CFileItem& item);
};
} // namespace GUILIB
} // namespace UTILS
