/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelNumber.h"

#include "utils/StringUtils.h"

using namespace PVR;

const char CPVRChannelNumber::SEPARATOR = '.';

std::string CPVRChannelNumber::FormattedChannelNumber() const
{
  return ToString(SEPARATOR);
}

std::string CPVRChannelNumber::SortableChannelNumber() const
{
  // Note: The subchannel separator is a character that does not work for a
  //       SortItem (at least not on all platforms). See SortUtils::Sort for
  //       details. Only numbers, letters and the blank are safe to use.
  return ToString(' ');
}

std::string CPVRChannelNumber::ToString(char separator) const
{
  if (m_iSubChannelNumber == 0)
    return StringUtils::Format("%u", m_iChannelNumber);
  else
    return StringUtils::Format("%u%c%u", m_iChannelNumber, separator, m_iSubChannelNumber);
}
