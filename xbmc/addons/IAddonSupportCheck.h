/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace ADDONS
{

/*!
 * @brief Parent class to ask addons for support.
 *
 * This one can be used as a parent on the respective Kodi-sided addon class,
 * this makes it easier to query for the desired support without using the
 * class's own function definition.
 */
class IAddonSupportCheck
{
public:
  IAddonSupportCheck() = default;
  virtual ~IAddonSupportCheck() = default;

  /*!
   * @brief Function to query the respective add-ons used for the support of
   * the desired file.
   *
   * @param[in] filename File which is queried for addon support
   * @return True if addon supports the desired file, false if not
   *
   * @note Is set here with true as default and not with "= 0" in order to have
   * class expandable and perhaps to be able to insert other query functions in
   * the future.
   */
  virtual bool SupportsFile(const std::string& filename) { return true; }
};

} /* namespace ADDONS */
} /* namespace KODI */
