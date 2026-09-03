/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RadioRDSLanguages.h"

#include <array>

namespace
{

// An index the standard reserves, which names no language
constexpr std::string_view RESERVED{};

/* see page 84, Annex J in the standard */
// clang-format off
constexpr std::array<std::string_view, KODI::RDS::LANGUAGE_INDEX_COUNT> LANGUAGE_CODES{{
  // 0         1         2         3         4         5         6         7         8         9         A         B         C         D         E         F
  RESERVED, "alb",    "bre",    "cat",    "hrv",    "wel",    "cze",    "dan",    "ger",    "eng",    "spa",    "epo",    "est",    "baq",    "fao",    "fre",    // 0
  "fry",    "gle",    "gla",    "glg",    "ice",    "ita",    "smi",    "lat",    "lav",    "ltz",    "lit",    "hun",    "mlt",    "dut",    "nor",    "oci",    // 1
  "pol",    "por",    "rum",    "rom",    "srp",    "slo",    "slv",    "fin",    "swe",    "tur",    "nld",    "wln",    RESERVED, RESERVED, RESERVED, RESERVED, // 2
  RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, // 3
  RESERVED, RESERVED, RESERVED, RESERVED, RESERVED, "zul",    "vie",    "uzb",    "urd",    "ukr",    "tha",    "tel",    "tat",    "tam",    "tgk",    "swa",    // 4
  "srn",    "som",    "sin",    "sna",    "hbs",    "rue",    "rus",    "que",    "pus",    "pan",    "per",    "pap",    "ori",    "nep",    "nde",    "mar",    // 5
  "rum",    "may",    "mlg",    "mkd",    "lao",    "kor",    "khm",    "kaz",    "kan",    "jpn",    "ind",    "hin",    "heb",    "hau",    "grn",    "guj",    // 6
  "gre",    "geo",    "ful",    "prs",    "chv",    "chi",    "bur",    "bul",    "ben",    "bel",    "bam",    "aze",    "asm",    "arm",    "ara",    "amh"     // 7
}};
// clang-format on

} // unnamed namespace

namespace KODI::RDS
{

std::string_view LanguageCode(unsigned int index)
{
  return index < LANGUAGE_INDEX_COUNT ? LANGUAGE_CODES[index] : RESERVED;
}

LANGUAGE::CLanguageTag Language(unsigned int index)
{
  const std::string_view code{LanguageCode(index)};
  if (code.empty())
    return LANGUAGE::CLanguageTag::Undetermined();

  return LANGUAGE::CLanguageTag::ParseStreamLanguage(std::string{code});
}

} // namespace KODI::RDS
