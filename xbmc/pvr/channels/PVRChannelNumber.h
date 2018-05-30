/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <string>

namespace PVR
{
  class CPVRChannelNumber
  {
  public:
    CPVRChannelNumber() = default;

    constexpr CPVRChannelNumber(unsigned int iChannelNumber, unsigned int iSubChannelNumber)
    : m_iChannelNumber(iChannelNumber), m_iSubChannelNumber(iSubChannelNumber) {}

    constexpr bool operator ==(const CPVRChannelNumber &right) const
    {
      return (m_iChannelNumber  == right.m_iChannelNumber &&
              m_iSubChannelNumber == right.m_iSubChannelNumber);
    }

    constexpr bool operator !=(const CPVRChannelNumber &right) const
    {
      return !(*this == right);
    }

    constexpr bool operator <(const CPVRChannelNumber &right) const
    {
      return m_iChannelNumber == right.m_iChannelNumber
        ? m_iSubChannelNumber < right.m_iSubChannelNumber
        : m_iChannelNumber < right.m_iChannelNumber;
    }

    /*!
     * @brief Check whether this channel number is valid (main channel number > 0).
     * @return True if valid, false otherwise..
     */
    constexpr bool IsValid() const { return m_iChannelNumber > 0; }

    /*!
     * @brief Set the primary channel number.
     * @return The channel number.
     */
    constexpr unsigned int GetChannelNumber() const
    {
      return m_iChannelNumber;
    }

    /*!
     * @brief Set the sub channel number.
     * @return The sub channel number (ATSC).
     */
    constexpr unsigned int GetSubChannelNumber() const
    {
      return m_iSubChannelNumber;
    }

    /*!
     * @brief The character used to separate channel and subchannel number.
     */
    static const char SEPARATOR; // '.'

    /*!
     * @brief Get a string representation for the channel number.
     * @return The formatted string in the form <channel>SEPARATOR<subchannel>.
     */
    std::string FormattedChannelNumber() const;

  private:
    unsigned int m_iChannelNumber = 0;
    unsigned int m_iSubChannelNumber = 0;
  };
}
