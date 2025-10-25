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
class CIso639_2
{
public:
  CIso639_2() = delete;

  /*!
   * \brief Retrieve the name of the language for the provided ISO 639-2 code.
   * \param[in] tCode alpha-3 ISO 639-2/T code
   * \return English name of the language, nullopt if no language by that code exists.
   */
  static std::optional<std::string> LookupByCode(std::string_view tCode);

  /*!
   * \brief Retrieve the name of the language for the provided ISO 639-2 code.
   * \param[in] longCode alpha-3 ISO 639-2/T code, coded as a 32 bit unsigned integer
   * \return English name of the language, nullopt if no language by that code exists.
   */
  static std::optional<std::string> LookupByCode(uint32_t longTcode);

  /*!
   * \brief Retrieve the code of the provided language name.
   * \param[in] name English language name
   * \return alpha-3 ISO 639-2/T code of the language, nullopt if no language by that name exists.
   */
  static std::optional<std::string> LookupByName(std::string_view name);

  /*!
   * \brief Provide a list of defined ISO 639-2 languages, including B/T variants
   * \param[in] langMap map to add languages to
   * \return true for success, false otherwise
   */
  static bool ListLanguages(std::map<std::string, std::string>& langMap);

  /*!
   * \brief Retrieve the ISO 639-2/T code for an ISO 639-2/B code
   * \param bCode the ISO 639-2/B code, coded as a 32 bit unsigned integer
   * \return The matching ISO 639-2/T code, nullopt if there isn't one.
   */
  static std::optional<uint32_t> BCodeToTCode(uint32_t bCode);

  /*!
   * \brief Retrieve the ISO 639-2/B code for an ISO 639-2/T code
   * \param tCode the ISO 639-2/T code, coded as a 32 bit unsigned integer
   * \return The matching ISO 639-2/B code, nullopt if there isn't one.
   */
  static std::optional<std::string> TCodeToBCode(std::string_view tCode);
};
} // namespace KODI::UTILS::I18N
