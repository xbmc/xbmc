/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/IPVRComponent.h"
#include "pvr/settings/PVRSettings.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <string>

class CFileItem;

namespace PVR
{
class CPVRGUIActionsUtils : public IPVRComponent
{
public:
  CPVRGUIActionsUtils();
  virtual ~CPVRGUIActionsUtils() = default;

  /*!
     * @brief Get the currently selected item path; used across several windows/dialogs to share
     * item selection.
     * @param bRadio True to query the selected path for PVR radio, false for Live TV.
     * @return the path.
     */
  std::string GetSelectedItemPath(bool bRadio) const;

  /*!
     * @brief Set the currently selected item path; used across several windows/dialogs to share
     * item selection.
     * @param bRadio True to set the selected path for PVR radio, false for Live TV.
     * @param path The new path to set.
     */
  void SetSelectedItemPath(bool bRadio, const std::string& path);

  /*!
     * @brief Process info action for the given item.
     * @param item The item.
     */
  bool OnInfo(const std::shared_ptr<CFileItem>& item);

private:
  CPVRGUIActionsUtils(const CPVRGUIActionsUtils&) = delete;
  CPVRGUIActionsUtils const& operator=(CPVRGUIActionsUtils const&) = delete;

  mutable CCriticalSection m_critSection;
  CPVRSettings m_settings;
  std::string m_selectedItemPathTV;
  std::string m_selectedItemPathRadio;
};

namespace GUI
{
// pretty scope and name
using Utils = CPVRGUIActionsUtils;
} // namespace GUI

} // namespace PVR
