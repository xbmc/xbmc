/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/IPVRComponent.h"

#include <memory>

class CFileItem;

namespace PVR
{
class CPVRGUIActionsUtils : public IPVRComponent
{
public:
  CPVRGUIActionsUtils() = default;
  ~CPVRGUIActionsUtils() override = default;

  /*!
     * @brief Check whether OnInfo supports the given item.
     * @param item The item.
     * @return True if supported, false otherwise.
     */
  bool HasInfoForItem(const CFileItem& item) const;

  /*!
     * @brief Process info action for the given item.
     * @param item The item.
     * @return True on success, false otherwise.
     */
  bool OnInfo(const CFileItem& item);

  /*!
   * @brief Load item details (create recording info tag etc.).
   * @param item The item.
   * @return Loaded item on success, nullptr otherwise.
   */
  std::shared_ptr<CFileItem> LoadItem(const CFileItem& item);

private:
  CPVRGUIActionsUtils(const CPVRGUIActionsUtils&) = delete;
  CPVRGUIActionsUtils const& operator=(CPVRGUIActionsUtils const&) = delete;
};

namespace GUI
{
// pretty scope and name
using Utils = CPVRGUIActionsUtils;
} // namespace GUI

} // namespace PVR
