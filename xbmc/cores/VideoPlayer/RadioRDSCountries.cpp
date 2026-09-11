/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RadioRDSCountries.h"

#include <array>

namespace
{

// A cell the standard reserves, which names no place
constexpr std::string_view RESERVED{};

using Row = std::array<std::string_view, KODI::RDS::EXTENDED_COUNTRY_CODE_COUNT>;
using Table = std::array<Row, KODI::RDS::COUNTRY_CODE_COUNT>;

/* page 71, Annex D, table D.1 in the standard and Annex N */
// clang-format off
// ECC 0xA0
constexpr Table COUNTRY_CODES_A{{
    {{"US"      , RESERVED  , "AI"      , "BO"      , "GT"      , RESERVED  , RESERVED}}, // 1
    {{"US"      , RESERVED  , "AG"      , "CO"      , "HN"      , RESERVED  , RESERVED}}, // 2
    {{"US"      , RESERVED  , "EC"      , "JM"      , "AW"      , RESERVED  , RESERVED}}, // 3
    {{"US"      , RESERVED  , "FK"      , "MQ"      , RESERVED  , RESERVED  , RESERVED}}, // 4
    {{"US"      , RESERVED  , "BB"      , "GF"      , "MS"      , RESERVED  , RESERVED}}, // 5
    {{"US"      , RESERVED  , "BZ"      , "PY"      , "TT"      , RESERVED  , RESERVED}}, // 6
    {{"US"      , RESERVED  , "KY"      , "NI"      , "PE"      , RESERVED  , RESERVED}}, // 7
    {{"US"      , RESERVED  , "CR"      , RESERVED  , "SR"      , RESERVED  , RESERVED}}, // 8
    {{"US"      , RESERVED  , "CU"      , "PA"      , "UY"      , RESERVED  , RESERVED}}, // 9
    {{"US"      , RESERVED  , "AR"      , "DM"      , "KN"      , RESERVED  , RESERVED}}, // A
    {{"US"      , "CA"      , "BR"      , "DO"      , "LC"      , "MX"      , RESERVED}}, // B
    {{RESERVED  , "CA"      , "BM"      , "CL"      , "SV"      , "VC"      , RESERVED}}, // C
    {{"US"      , "CA"      , "AN"      , "GD"      , "HT"      , "MX"      , RESERVED}}, // D
    {{"US"      , "CA"      , "GP"      , "TC"      , "VE"      , "MX"      , RESERVED}}, // E
    {{RESERVED  , "GL"      , "BS"      , "GY"      , RESERVED  , "VG"      , "PM"}}, // F
}};

// ECC 0xD0
constexpr Table COUNTRY_CODES_D{{
    {{"CM"      , "NA"      , "SL"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // 1
    {{"CF"      , "LR"      , "ZW"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // 2
    {{"DJ"      , "GH"      , "MZ"      , "EH"      , RESERVED  , RESERVED  , RESERVED}}, // 3
    {{"MG"      , "MR"      , "UG"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // 4
    {{"ML"      , "ST"      , "SZ"      , "RW"      , RESERVED  , RESERVED  , RESERVED}}, // 5
    {{"AO"      , "CV"      , "KE"      , "LS"      , RESERVED  , RESERVED  , RESERVED}}, // 6
    {{"GQ"      , "SN"      , "SO"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // 7
    {{"GA"      , "GM"      , "NE"      , "SC"      , RESERVED  , RESERVED  , RESERVED}}, // 8
    {{"GN"      , "BI"      , "TD"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // 9
    {{"ZA"      , "AC"      , "GW"      , "MU"      , RESERVED  , RESERVED  , RESERVED}}, // A
    {{"BF"      , "BW"      , "ZR"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // B
    {{"CG"      , "KM"      , "CI"      , "SD"      , RESERVED  , RESERVED  , RESERVED}}, // C
    {{"TG"      , "TZ"      , "Zanzibar", RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // D
    {{"BJ"      , "ET"      , "ZM"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // E
    {{"MW"      , "NG"      , RESERVED  , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // F
}};

// ECC 0xE0
constexpr Table COUNTRY_CODES_E{{
    {{"DE"      , "GR"      , "MA"      , RESERVED  , "MD"      , RESERVED  , RESERVED}}, // 1
    {{"DZ"      , "CY"      , "CZ"      , "IE"      , "EE"      , RESERVED  , RESERVED}}, // 2
    {{"AD"      , "SM"      , "PL"      , "TR"      , "KG"      , RESERVED  , RESERVED}}, // 3
    {{"IL"      , "CH"      , "VA"      , "MK"      , RESERVED  , RESERVED  , RESERVED}}, // 4
    {{"IT"      , "JO"      , "SK"      , "TJ"      , RESERVED  , RESERVED  , RESERVED}}, // 5
    {{"BE"      , "FI"      , "SY"      , RESERVED  , "UA"      , RESERVED  , RESERVED}}, // 6
    {{"RU"      , "LU"      , "TN"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // 7
    {{"PS"      , "BG"      , RESERVED  , "NL"      , "PT"      , RESERVED  , RESERVED}}, // 8
    {{"AL"      , "DK"      , "LI"      , "LV"      , "SI"      , RESERVED  , RESERVED}}, // 9
    {{"AT"      , "GI"      , "IS"      , "LB"      , "AM"      , RESERVED  , RESERVED}}, // A
    {{"HU"      , "IQ"      , "MC"      , "AZ"      , "UZ"      , RESERVED  , RESERVED}}, // B
    {{"MT"      , "GB"      , "LT"      , "HR"      , "GE"      , RESERVED  , RESERVED}}, // C
    {{"DE"      , "LY"      , "YU"      , "KZ"      , RESERVED  , RESERVED  , RESERVED}}, // D
    {{RESERVED  , "RO"      , "ES"      , "SE"      , "TM"      , RESERVED  , RESERVED}}, // E
    {{"EG"      , "FR"      , "NO"      , "BY"      , "BA"      , RESERVED  , RESERVED}}, // F
}};

// ECC 0xF0
constexpr Table COUNTRY_CODES_F{{
    {{"AU"      , "KI"      , "KW"      , "LA"      , RESERVED  , RESERVED  , RESERVED}}, // 1
    {{"AU"      , "BT"      , "QA"      , "TH"      , RESERVED  , RESERVED  , RESERVED}}, // 2
    {{"AU"      , "BD"      , "KH"      , "TO"      , RESERVED  , RESERVED  , RESERVED}}, // 3
    {{"AU"      , "PK"      , "WS"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // 4
    {{"AU"      , "FJ"      , "IN"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // 5
    {{"AU"      , "OM"      , "MO"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // 6
    {{"AU"      , "NR"      , "VN"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // 7
    {{"AU"      , "IR"      , "PH"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // 8
    {{"SA"      , "NZ"      , "JP"      , "PG"      , RESERVED  , RESERVED  , RESERVED}}, // 9
    {{"AF"      , "SB"      , "SG"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // A
    {{"MM"      , "BN"      , "MV"      , "YE"      , RESERVED  , RESERVED  , RESERVED}}, // B
    {{"CN"      , "LK"      , "ID"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // C
    {{"KP"      , "TW"      , "AE"      , RESERVED  , RESERVED  , RESERVED  , RESERVED}}, // D
    {{"BH"      , "KR"      , "NP"      , "FM"      , RESERVED  , RESERVED  , RESERVED}}, // E
    {{"MY"      , "HK"      , "VU"      , "MN"      , RESERVED  , RESERVED  , RESERVED}}, // F
}};
// clang-format on

} // unnamed namespace

namespace KODI::RDS
{

std::optional<std::string_view> CountryCode(unsigned int extendedCountryCode,
                                            unsigned int countryCode,
                                            unsigned int index)
{
  if (countryCode < 1 || countryCode > COUNTRY_CODE_COUNT || index >= EXTENDED_COUNTRY_CODE_COUNT)
    return std::nullopt;

  const Table* table{nullptr};
  switch (extendedCountryCode)
  {
    case 0xA0:
      table = &COUNTRY_CODES_A;
      break;
    case 0xD0:
      table = &COUNTRY_CODES_D;
      break;
    case 0xE0:
      table = &COUNTRY_CODES_E;
      break;
    case 0xF0:
      table = &COUNTRY_CODES_F;
      break;
    default:
      return std::nullopt;
  }

  return (*table)[countryCode - 1][index];
}

std::optional<LANGUAGE::CTerritory> Country(unsigned int extendedCountryCode,
                                            unsigned int countryCode,
                                            unsigned int index)
{
  const auto code{CountryCode(extendedCountryCode, countryCode, index)};
  if (!code.has_value())
    return std::nullopt;

  // A reserved cell, and one holding text that names no region, both name nowhere. FromCode
  // answers the second: what counts as a region is the subtag registry's answer, not a second one.
  return LANGUAGE::CTerritory::FromCode(*code);
}

} // namespace KODI::RDS
