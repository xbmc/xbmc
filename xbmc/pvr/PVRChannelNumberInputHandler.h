/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/channels/PVRChannelNumber.h"
#include "threads/CriticalSection.h"
#include "threads/Timer.h"
#include "utils/EventStream.h"

#include <string>
#include <vector>

namespace PVR
{
struct PVRChannelNumberInputChangedEvent
{
  explicit PVRChannelNumberInputChangedEvent(const std::string& input) : m_input(input) {}
  virtual ~PVRChannelNumberInputChangedEvent() = default;

  std::string m_input;
};

class CPVRChannelNumberInputHandler : private ITimerCallback
{
public:
  CPVRChannelNumberInputHandler();
  ~CPVRChannelNumberInputHandler() override = default;

  /*!
   * @brief Get the events available for CEventStream.
   * @return The events.
   */
  CEventStream<PVRChannelNumberInputChangedEvent>& Events() { return m_events; }

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

  mutable CCriticalSection m_mutex;

private:
  void ExecuteAction();

  void SetLabel(const std::string& label);

  std::vector<std::string> m_sortedChannelNumbers;
  const uint32_t m_delay;
  std::string m_inputBuffer;
  std::string m_label;
  CTimer m_timer;
  CEventSource<PVRChannelNumberInputChangedEvent> m_events;
};

} // namespace PVR
