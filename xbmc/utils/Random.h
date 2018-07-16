/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <random>

namespace KODI
{
namespace UTILS
{
template<class TIterator>
void RandomShuffle(TIterator begin, TIterator end)
{
  std::random_device rd;
  std::mt19937 mt(rd());
  std::shuffle(begin, end, mt);
}
}
}
