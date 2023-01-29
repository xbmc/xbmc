/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CGUIComponent;

namespace KODI
{
namespace RETRO
{
class CRPProcessInfo;
class ISavestate;

/*!
 * \brief Class to send messages to the GUI, if a GUI is present
 */
class CGUIGameMessenger
{
public:
  CGUIGameMessenger(CRPProcessInfo& processInfo);

  /*!
   * \brief Refresh savestate GUI elements being displayed
   *
   * \param savestatePath The savestate to refresh, or empty to refresh all savestates
   * \param savestate Optional savestate info to send with the message
   */
  void RefreshSavestates(const std::string& savestatePath = "", ISavestate* savestate = nullptr);

private:
  CGUIComponent* const m_guiComponent;
};
} // namespace RETRO
} // namespace KODI
