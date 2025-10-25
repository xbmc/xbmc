/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace KODI::UTILS::I18N
{
class CIso639_1
{
public:
  CIso639_1() = delete;

  /*!
   * \brief Retrieve the name of the language for the provided ISO 639-1 code.
   * \param[in] code alpha-2 ISO 639-1 code
   * \return English name of the language, nullopt if no language by that code exists.
   */
  static std::optional<std::string> LookupByCode(std::string_view code);

  /*!
   * \brief Retrieve the name of the language for the provided ISO 639-1 code.
   * \param[in] longCode alpha-2 ISO 639-1 code, coded as a 32 bit unsigned integer
   * \return English name of the language, nullopt if no language by that code exists.
   */
  static std::optional<std::string> LookupByCode(uint32_t longCode);

  /*!
   * \brief Retrieve the code of the provided language name.
   * \param[in] name English language name
   * \return alpha-2 code of the language, nullopt if no language by that name exists.
   */
  static std::optional<std::string> LookupByName(std::string_view name);

  /*!
   * \brief Provide a list of defined ISO 639-1 languages
   * \param[in] langMap map to add languages to
   * \return true for success, false otherwise
   */
  static bool ListLanguages(std::map<std::string, std::string>& langMap);
};
} // namespace KODI::UTILS::I18N
