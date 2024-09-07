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
class CPVRMediaTag;

class CPVRGUIActionsMedia : public IPVRComponent
{
public:
  CPVRGUIActionsMedia() = default;
  ~CPVRGUIActionsMedia() override = default;

  /*!
   * @brief Open a dialog with information for a given media tag.
   * @param item containing a media tag.
   * @return true on success, false otherwise.
   */
  bool ShowMediaTagInfo(const CFileItem& item) const;

private:
  CPVRGUIActionsMedia(const CPVRGUIActionsMedia&) = delete;
  CPVRGUIActionsMedia const& operator=(CPVRGUIActionsMedia const&) = delete;

  /*!
   * @brief Open the media tag settings dialog.
   * @param media tag containing the media tag the settings shall be displayed for.
   * @return true, if the dialog was ended successfully, false otherwise.
   */
  bool ShowMediaTagSettings(const std::shared_ptr<CPVRMediaTag>& mediaTag) const;
};

namespace GUI
{
// pretty scope and name
using Media = CPVRGUIActionsMedia;
} // namespace GUI

} // namespace PVR
