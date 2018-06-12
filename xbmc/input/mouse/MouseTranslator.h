/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "MouseTypes.h"

#include <stdint.h>

class TiXmlElement;

class CMouseTranslator
{
public:
  /*!
   * \brief Translate a keymap element to a key ID
   */
  static uint32_t TranslateCommand(const TiXmlElement *pButton);

  /*!
   * \brief Translate a mouse event ID to a mouse button index
   *
   * \param eventId The event ID from MouseStat.h
   * \param[out] buttonId The button ID from MouseTypes.h, or unmodified if unsuccessful
   *
   * \return True if successful, false otherwise
   */
  static bool TranslateEventID(unsigned int eventId, KODI::MOUSE::BUTTON_ID &buttonId);
};
