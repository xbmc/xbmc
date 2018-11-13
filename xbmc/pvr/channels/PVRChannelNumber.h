/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

    /*!
     * @brief Get a string representation for the channel number that can be used for SortItems.
     * @return The sortable string in the form <channel> <subchannel>.
     */
    std::string SortableChannelNumber() const;

  private:
    std::string ToString(char separator) const;

    unsigned int m_iChannelNumber = 0;
    unsigned int m_iSubChannelNumber = 0;
  };
}
