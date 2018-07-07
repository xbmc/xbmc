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

#pragma once

#include <vector>
#include <string>

#include "threads/CriticalSection.h"
#include "threads/Timer.h"

#include "pvr/channels/PVRChannelNumber.h"

namespace PVR
{

class CPVRChannelNumberInputHandler : private ITimerCallback
{
public:
  static const int CHANNEL_NUMBER_INPUT_MAX_DIGITS = 5;

  CPVRChannelNumberInputHandler();

  /*!
   * @brief ctor.
   * @param iDelay timer delay in millisecods.
   * @param iMaxDigits maximum number of display digits to use.
   */
  CPVRChannelNumberInputHandler(int iDelay, int iMaxDigits = CHANNEL_NUMBER_INPUT_MAX_DIGITS);

  ~CPVRChannelNumberInputHandler() override = default;

  // implementation of ITimerCallback
  void OnTimeout() override;

  /*!
   * @brief Get the currently available channel numbers.
   * @param channelNumbers The list to fill with the channel numbers.
   */
  virtual void GetChannelNumbers(std::vector<std::string>& channelNumbers) = 0;

  /*!
   * @brief This method gets called after the channel number input timer has expired.
   */
  virtual void OnInputDone() = 0;

  /*!
   * @brief Appends a channel number character.
   * @param cCharacter The character to append. value must be CPVRChannelNumber::SEPARATOR ('.') or any char in the range from '0' to '9'.
   */
  virtual void AppendChannelNumberCharacter(char cCharacter);

  /*!
   * @brief Check whether a channel number was entered.
   * @return True if the handler currently holds a channel number, false otherwise.
   */
  bool HasChannelNumber() const;

  /*!
   * @brief Get the currently entered channel number as a formatted string.
   * @return the channel number string.
   */
  std::string GetChannelNumberLabel() const;

  /*!
   * @brief If a number was entered, execute the associated action.
   * @return True, if the action was executed, false otherwise.
   */
  bool CheckInputAndExecuteAction();

protected:
  /*!
   * @brief Get the currently entered channel number.
   * @return the channel number.
   */
  CPVRChannelNumber GetChannelNumber() const;

  /*!
   * @brief Get the currently entered number of digits.
   * @return the number of digits.
   */
  size_t GetCurrentDigitCount() const { return m_inputBuffer.size(); }

  CCriticalSection m_mutex;

private:
  void ExecuteAction();

  std::vector<std::string> m_sortedChannelNumbers;
  const int m_iDelay;
  const int m_iMaxDigits;
  std::string m_inputBuffer;
  std::string m_label;
  CTimer m_timer;
};

} // namespace PVR
