/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "PVRChannelNumber.h"

#include "utils/StringUtils.h"

using namespace PVR;

const char CPVRChannelNumber::SEPARATOR = '.';

CPVRChannelNumber& CPVRChannelNumber::operator=(const CPVRChannelNumber &channelNumber)
{
  m_iChannelNumber = channelNumber.m_iChannelNumber;
  m_iSubChannelNumber = channelNumber.m_iSubChannelNumber;
  return *this;
}

bool CPVRChannelNumber::operator==(const CPVRChannelNumber &right) const
{
  return (m_iChannelNumber  == right.m_iChannelNumber &&
          m_iSubChannelNumber == right.m_iSubChannelNumber);
}

bool CPVRChannelNumber::operator!=(const CPVRChannelNumber &right) const
{
  return !(*this == right);
}

bool CPVRChannelNumber::operator <(const CPVRChannelNumber &right) const
{
  if (m_iChannelNumber == right.m_iChannelNumber)
    return m_iSubChannelNumber < right.m_iSubChannelNumber;

  return m_iChannelNumber < right.m_iChannelNumber;
}

unsigned int CPVRChannelNumber::GetChannelNumber() const
{
  return m_iChannelNumber;
}

unsigned int CPVRChannelNumber::GetSubChannelNumber() const
{
  return m_iSubChannelNumber;
}

std::string CPVRChannelNumber::FormattedChannelNumber() const
{
  if (m_iSubChannelNumber == 0)
    return StringUtils::Format("%u", m_iChannelNumber);
  else
    return StringUtils::Format("%u%c%u", m_iChannelNumber, SEPARATOR, m_iSubChannelNumber);
}
