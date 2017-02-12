#pragma once
/*
 *      Copyright (C) 2012-2017 Team Kodi
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

#include <string>
#include <deque>

#include "threads/CriticalSection.h"
#include "threads/Timer.h"

namespace PVR
{

class CPVRChannelNumberInputHandler : private ITimerCallback
{
public:
  static const int CHANNEL_NUMBER_INPUT_MAX_DIGITS = 4;

  CPVRChannelNumberInputHandler();

  /*!
   * @brief ctor.
   * @param iDelay timer delay in millisecods.
   * @param iMaxDigits maximum number of display digits to use.
   */
  CPVRChannelNumberInputHandler(int iDelay, int iMaxDigits = CHANNEL_NUMBER_INPUT_MAX_DIGITS);

  virtual ~CPVRChannelNumberInputHandler() = default;

  // implementation of ITimerCallback
  void OnTimeout() override;

  /*!
   * @brief This method gets called after the channel number input timer has expired.
   */
  virtual void OnInputDone() = 0;

  /*!
   * @brief Appends a channel digit.
   * @param iDigit the digit to append. value must be in range of 0 to 9.
   */
  void AppendChannelNumberDigit(int iDigit);

  /*!
   * @brief Get the currently entered channel number as a formatted string. Format is n digits with leading zeros, where n is the number of digits specified when calling the ctor.
   * @return the channel number string.
   */
  std::string GetChannelNumberAsString() const;

protected:
  /*!
   * @brief Get the currently entered channel number.
   * @return the channel number.
   */
  int GetChannelNumber() const;

  /*!
   * @brief Get the currently entered number of digits.
   * @return the number of digits.
   */
  int GetCurrentDigitCount() const { return m_digits.size(); }

  CCriticalSection m_mutex;

private:
  const int m_iDelay;
  const int m_iMaxDigits;
  std::deque<int> m_digits;
  std::string m_strChannel;
  CTimer m_timer;
};

} // namespace PVR
