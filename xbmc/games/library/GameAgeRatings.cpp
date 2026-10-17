/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameAgeRatings.h"

#include "utils/StringUtils.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string_view>

using namespace KODI;
using namespace GAME;

namespace
{
struct Classification
{
  std::string_view board;
  std::string_view value;
  int age;
};

// Youngest first within a board, and the usual spelling of an age before any
// other, so that the first match for an age is the one to show. Numbered
// classifications only need listing where their number would fit another age,
// or for ClassificationFor(). The letters were checked against the PEGI ratings
// of games that carry both.
constexpr std::array CLASSIFICATIONS{
    Classification{"PEGI", "3", 3},        Classification{"PEGI", "7", 7},
    Classification{"PEGI", "12", 12},      Classification{"PEGI", "16", 16},
    Classification{"PEGI", "18", 18},      Classification{"BBFC", "U", 3},
    Classification{"BBFC", "PG", 7},       Classification{"BBFC", "12", 12},
    Classification{"BBFC", "12A", 12},     Classification{"BBFC", "15", 16},
    Classification{"BBFC", "18", 18},      Classification{"BBFC", "R18", 18},
    Classification{"ESRB", "E", 3},        Classification{"ESRB", "EC", 3},
    Classification{"ESRB", "KA", 3},       Classification{"ESRB", "E10+", 7},
    Classification{"ESRB", "T", 12},       Classification{"ESRB", "M", 16},
    Classification{"ESRB", "AO", 18},      Classification{"USK", "0", 3},
    Classification{"USK", "6", 7},         Classification{"USK", "12", 12},
    Classification{"USK", "16", 16},       Classification{"USK", "18", 18},
    Classification{"CERO", "A", 3},        Classification{"CERO", "B", 12},
    Classification{"CERO", "C", 16},       Classification{"CERO", "D", 18},
    Classification{"CERO", "Z", 18},       Classification{"ACB", "G", 3},
    Classification{"ACB", "PG", 7},        Classification{"ACB", "M", 12},
    Classification{"ACB", "MA15+", 16},    Classification{"ACB", "R18+", 18},
    Classification{"ACB", "RC", 18},       Classification{"GRAC", "All", 3},
    Classification{"GRAC", "12", 12},      Classification{"GRAC", "15", 16},
    Classification{"GRAC", "18", 18},      Classification{"CLASS_IND", "L", 3},
    Classification{"CLASS_IND", "10", 12}, Classification{"CLASS_IND", "12", 12},
    Classification{"CLASS_IND", "14", 16}, Classification{"CLASS_IND", "16", 16},
    Classification{"CLASS_IND", "18", 18}, Classification{"GRB", "All", 3},
    Classification{"DJCTQ", "L", 3},       Classification{"SELL", "TP", 3},
    Classification{"OFLC", "G", 3},        Classification{"VRC", "GA", 3},
    Classification{"HSRS", "GA", 3},       Classification{"HSRS", "AD", 18},
    Classification{"AAMA", "Green", 3},    Classification{"AAMA", "Yellow", 12},
    Classification{"AAMA", "Red", 18},     Classification{"Tectoy", "TI", 3},
    Classification{"SEGA", "G", 3},        Classification{"SEGA", "X", 18},
};

// The age a written number fits: the nearest, or the older of two as near
int NearestAge(int years)
{
  int best = CGameAgeRatings::AGES.front();
  for (const int age : CGameAgeRatings::AGES)
  {
    if (std::abs(age - years) <= std::abs(best - years))
      best = age;
  }
  return best;
}
} // namespace

int CGameAgeRatings::AgeFor(const std::string& board, const std::string& value)
{
  for (const Classification& known : CLASSIFICATIONS)
  {
    if (StringUtils::EqualsNoCase(known.board, board) &&
        StringUtils::EqualsNoCase(known.value, value))
      return known.age;
  }

  const auto digit = std::ranges::find_if(value, [](char c)
                                          { return std::isdigit(static_cast<unsigned char>(c)); });
  if (digit == value.end())
    return 0;

  return NearestAge(std::atoi(&*digit));
}

std::string CGameAgeRatings::ClassificationFor(const std::string& board, int age)
{
  for (const Classification& known : CLASSIFICATIONS)
  {
    if (StringUtils::EqualsNoCase(known.board, board) && known.age >= age)
      return std::string(known.value);
  }
  return "";
}

int CGameAgeRatings::AgeOf(const std::vector<GameAgeRating>& ratings)
{
  for (const GameAgeRating& rating : ratings)
  {
    const int age = AgeFor(rating.board, rating.value);
    if (age > 0)
      return age;
  }
  return 0;
}
