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

namespace KODI::SETTINGS
{
/*!
 * \brief Outcome of a setting migration operation.
 */
enum class SettingConversionResult
{
  FAILURE, ///< Failed conversion, irrecoverable inconsistent state.
  NOT_PRESENT, ///< The old setting ID could not be located.
  INVALID, ///< The old setting has a value incompatible with its data type.
  CONVERTED, ///< The conversion was successful.
};

struct SettingBoolToIntMapping
{
  int m_default;
  int m_false;
  int m_true;
};

SettingConversionResult ConvertSettingBoolToInt(TiXmlElement* root,
                                                std::string_view oldSettingId,
                                                std::string_view newSettingId,
                                                const SettingBoolToIntMapping& mapping);

} // namespace KODI::SETTINGS
