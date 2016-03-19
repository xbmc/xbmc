/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRSettings.h"

#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

using namespace PVR;

void CPVRSettings::MarginTimeFiller(
  const CSetting * /*setting*/, std::vector< std::pair<std::string, int> > &list, int &current, void * /*data*/)
{
  list.clear();

  static const int marginTimeValues[] =
  {
    0, 1, 3, 5, 10, 15, 20, 30, 60, 90, 120, 180 // minutes
  };
  static const size_t marginTimeValuesCount = sizeof(marginTimeValues) / sizeof(int);

  for (size_t i = 0; i < marginTimeValuesCount; ++i)
  {
    int iValue = marginTimeValues[i];
    list.push_back(
      std::make_pair(StringUtils::Format(g_localizeStrings.Get(14044).c_str(), iValue) /* %i min */, iValue));
  }
}
