/*
 *  Copyright (C) 2025-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/i18n/Bcp47Common.h"

#include <string>

namespace KODI::UTILS::I18N
{
class CBcp47;

class CBcp47Formatter
{
public:
  /*!
   * \brief Create a BCP47 formatter
   * \param[in] style Format of the output
   */
  CBcp47Formatter(Bcp47FormattingStyle style) : m_style(style) {}

  /*!
   * \brief Format a tag to text according to the previously chosen format
   * \param[in] tag The tag to format
   * \return The formatted tag
   */
  std::string Format(const CBcp47& tag) const;

  /*!
   * \brief Format a tag to text according to the provided format
   * \param[in] tag The tag to format
   * \param[in] style Format of the output
   * \return The formatted tag
   */
  static std::string Format(const CBcp47& tag, Bcp47FormattingStyle style)
  {
    CBcp47Formatter formatter(style);
    return formatter.Format(tag);
  }

private:
  bool AppendLanguage(const CBcp47& tag, std::string& str) const;
  bool AppendExtLangs(const CBcp47& tag, std::string& str) const;
  bool AppendScript(const CBcp47& tag, std::string& str) const;
  bool AppendRegion(const CBcp47& tag, std::string& str) const;
  bool AppendVariants(const CBcp47& tag, std::string& str) const;
  bool AppendExtensions(const CBcp47& tag, std::string& str) const;
  bool AppendPrivateUse(const CBcp47& tag, std::string& str) const;
  bool AppendGrandfathered(const CBcp47& tag, std::string& str) const;
  void AppendDebugHeader(const CBcp47& tag, std::string& str) const;

  Bcp47FormattingStyle m_style{Bcp47FormattingStyle::FORMAT_BCP47};
};
} // namespace KODI::UTILS::I18N
