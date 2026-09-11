/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>

namespace KODI::LANGUAGE::I18N
{
class CIso639_1
{
public:
  CIso639_1() = delete;

  /*!
   * \brief Provide a list of defined ISO 639-1 languages
   * \param[in] langMap map to add languages to
   * \return true for success, false otherwise
   */
  static bool ListLanguages(std::map<std::string, std::string>& langMap);

  /*!
   * \brief Provide every name an ISO 639-1 language is known by, mapped to its code
   * \param[in] nameMap map to add the names to; an entry already present is kept
   * \return true for success, false otherwise
   */
  static bool ListLanguageNames(std::map<std::string, std::string>& nameMap);
};
} // namespace KODI::LANGUAGE::I18N
