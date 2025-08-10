/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/XBMCTinyXML.h"

#include <string_view>

class CNfoUtils
{
public:
  CNfoUtils() = delete;

  /*!
   * \brief Bring the XML representation of a NFO file up to date.
   * \param[in] root Root of the XML document
   * \return true for success (successful upgrade, already up to date, not versioned, ...)
   *         and false for any error.
   */
  static bool Upgrade(TiXmlElement* root);

  /*!
   * \brief Add a <version> attribute to the element \p elem. The element value is the current
   *        version of the provided NFO type \p tag.
   * \param[in] elem Element to add to.
   * \param[in] tag NFO type (ex. movie, tvshow)
   */
  static void SetVersion(TiXmlElement& elem, std::string_view tag);
};
