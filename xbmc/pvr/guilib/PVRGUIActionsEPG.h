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
#include <string>

class CFileItem;

namespace PVR
{
class CPVREpgInfoTag;

class CPVRGUIActionsEPG : public IPVRComponent
{
public:
  CPVRGUIActionsEPG() = default;
  ~CPVRGUIActionsEPG() override = default;

  /*!
   * @brief Open a dialog with epg information for a given item.
   * @param item containing epg data to show. item must be an epg tag, a channel or a timer.
   * @return true on success, false otherwise.
   */
  bool ShowEPGInfo(const CFileItem& item) const;

  /*!
   * @brief Open a dialog with the epg list for a given item.
   * @param item containing channel info. item must be an epg tag, a channel or a timer.
   * @return true on success, false otherwise.
   */
  bool ShowChannelEPG(const CFileItem& item) const;

  /*!
   * @brief Open a window containing a list of epg tags 'similar' to a given item.
   * @param item containing epg data for matching. item must be an epg tag, a channel or a
   * recording.
   * @return true on success, false otherwise.
   */
  bool FindSimilar(const CFileItem& item) const;

  /*!
   * @brief Execute a saved search. Displays result in search window if it is open.
   * @param item The item containing a search filter.
   * @return True on success, false otherwise.
   */
  bool ExecuteSavedSearch(const CFileItem& item);

  /*!
   * @brief Edit a saved search. Opens the search dialog.
   * @param item The item containing a search filter.
   * @return True on success, false otherwise.
   */
  bool EditSavedSearch(const CFileItem& item);

  /*!
   * @brief Rename a saved search. Opens a title input dialog.
   * @param item The item containing a search filter.
   * @return True on success, false otherwise.
   */
  bool RenameSavedSearch(const CFileItem& item);

  /*!
   * @brief Delete a saved search. Opens confirmation dialog before deleting.
   * @param item The item containing a search filter.
   * @return True on success, false otherwise.
   */
  bool DeleteSavedSearch(const CFileItem& item);

  /*!
   * @brief Get the title for the given EPG tag, taking settings and parental controls into account.
   * @param item The EPG tag.
   * @return "Parental locked" in case the access to the information is restricted by parental
   * controls, "No information avilable" if the real EPG event title is empty and
   * SETTING_EPG_HIDENOINFOAVAILABLE is false or tag is nullptr, the real EPG event title as given
   * by the EPG data provider otherwise.
   */
  std::string GetTitleForEpgTag(const std::shared_ptr<const CPVREpgInfoTag>& tag);

private:
  CPVRGUIActionsEPG(const CPVRGUIActionsEPG&) = delete;
  CPVRGUIActionsEPG const& operator=(CPVRGUIActionsEPG const&) = delete;
};

namespace GUI
{
// pretty scope and name
using EPG = CPVRGUIActionsEPG;
} // namespace GUI

} // namespace PVR
