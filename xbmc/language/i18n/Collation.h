/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI::LANGUAGE
{
class CLanguageTag;
} // namespace KODI::LANGUAGE

namespace KODI::LANGUAGE::I18N
{

/*!
 * \brief The collation weight a language gives a letter its alphabet places after z, where the
 *        generic accent folding would treat it as a variant of a or o.
 *
 * \todo Remove with the accent-folding fallback, once every platform collates by locale.
 *
 * \param[in] language The language whose alphabet orders the comparison.
 * \param[in] codepoint The upper or lower case codepoint being weighted.
 * \return A weight that sorts after 'z', or 0 where the language gives the codepoint no order
 *         of its own.
 */
[[nodiscard]] wchar_t NordicCollationWeight(const CLanguageTag& language,
                                            wchar_t codepoint) noexcept;

} // namespace KODI::LANGUAGE::I18N
