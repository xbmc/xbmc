/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MouseTypes.h"

#include <stdint.h>

namespace tinyxml2
{
class XMLElement;
}

/*!
 * \ingroup mouse
 */
class CMouseTranslator
{
public:
  /*!
   * \brief Translate a keymap element to a key ID
   */
  static uint32_t TranslateCommand(const tinyxml2::XMLElement* pButton);

  /*!
   * \brief Translate a mouse event ID to a mouse button index
   *
   * \param eventId The event ID from MouseStat.h
   * \param[out] buttonId The button ID from MouseTypes.h, or unmodified if unsuccessful
   *
   * \return True if successful, false otherwise
   */
  static bool TranslateEventID(unsigned int eventId, KODI::MOUSE::BUTTON_ID& buttonId);
};
