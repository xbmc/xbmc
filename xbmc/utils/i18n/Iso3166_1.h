/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace KODI::UTILS::I18N
{
class CIso3166_1
{
public:
  CIso3166_1() = delete;

  /*!
   * \brief Retrieve the alpha-3 ISO 3166-1 code for the provided alpha-2 code.
   * \param[in] code lowercase alpha-2 code
   * \return alpha-3 code or nullopt for an unknown alpha-2 code.
   */
  static std::optional<std::string> Alpha2ToAlpha3(std::string_view code);

  /*!
   * \brief Retrieve the alpha-2 ISO 3166-1 code for the provided alpha-3 code.
   * \param[in] code lowercase alpha-3 code
   * \return alpha-2 code or nullopt for an unknown alpha-3 code.
   */
  static std::optional<std::string> Alpha3ToAlpha2(std::string_view code);

  /*!
   * \brief Existence of the provided alpha-3 code in the collection of ISO 3166-1 regions.
   * \param[in] code lowercase alpha-3 code
   * \return true for an existing alpha-3 code, false otherwise.
   */
  static bool ContainsAlpha3(std::string_view code);
};
} // namespace KODI::UTILS::I18N
