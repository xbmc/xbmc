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

#include "PVRChannelNumberInputHandler.h"

#include <algorithm>
#include <cstdlib>

#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"

namespace PVR
{

CPVRChannelNumberInputHandler::CPVRChannelNumberInputHandler()
: CPVRChannelNumberInputHandler(g_advancedSettings.m_iPVRNumericChannelSwitchTimeout, CHANNEL_NUMBER_INPUT_MAX_DIGITS)
{
}

CPVRChannelNumberInputHandler::CPVRChannelNumberInputHandler(int iDelay, int iMaxDigits /* = CHANNEL_NUMBER_INPUT_MAX_DIGITS */)
: m_iDelay(iDelay),
  m_iMaxDigits(iMaxDigits),
  m_timer(this)
{
}

void CPVRChannelNumberInputHandler::OnTimeout()
{
  if (m_inputBuffer.empty())
  {
    CSingleLock lock(m_mutex);
    m_label.erase();
  }
  else
  {
    // call the overridden worker method
    OnInputDone();

    CSingleLock lock(m_mutex);

    // erase input buffer immediately , but...
    m_inputBuffer.erase();

    // ... display the label for another .5 secs if we stopped the timer before regular timeout.
    if (m_timer.IsRunning())
      m_label.erase();
    else
      m_timer.Start(500);
  }
}

void CPVRChannelNumberInputHandler::ExecuteAction()
{
  m_timer.Stop();
  OnTimeout();
}

bool CPVRChannelNumberInputHandler::CheckInputAndExecuteAction()
{
  const CPVRChannelNumber channelNumber = GetChannelNumber();
  if (channelNumber.IsValid())
  {
    // we have a valid channel number; execute the associated action now.
    ExecuteAction();
    return true;
  }
  return false;
}

void CPVRChannelNumberInputHandler::AppendChannelNumberCharacter(char cCharacter)
{
  if (cCharacter != CPVRChannelNumber::SEPARATOR && (cCharacter < '0' || cCharacter > '9'))
    return;

  CSingleLock lock(m_mutex);

  if (cCharacter == CPVRChannelNumber::SEPARATOR)
  {
    // no leading separator
    if (m_inputBuffer.empty())
      return;

    // max one separator
    if (m_inputBuffer.find(CPVRChannelNumber::SEPARATOR) != std::string::npos)
      return;
  }

  if (m_inputBuffer.size() == static_cast<size_t>(m_iMaxDigits))
  {
    m_inputBuffer.erase(m_inputBuffer.begin());
    m_label = m_inputBuffer;
  }
  else if (m_inputBuffer.empty())
  {
    m_sortedChannelNumbers.clear();
    GetChannelNumbers(m_sortedChannelNumbers);

    std::sort(m_sortedChannelNumbers.begin(), m_sortedChannelNumbers.end());
  }

  m_inputBuffer.append(&cCharacter, 1);
  m_label = m_inputBuffer;

  for (auto it = m_sortedChannelNumbers.begin(); it != m_sortedChannelNumbers.end();)
  {
    const std::string channel = *it;
    ++it;

    if (StringUtils::StartsWith(channel, m_inputBuffer))
    {
      if (it != m_sortedChannelNumbers.end() && StringUtils::StartsWith(*it, m_inputBuffer))
      {
        // there are alternative numbers; wait for more input
        break;
      }

      // no alternatives; complete the number and fire immediately
      m_inputBuffer = channel;
      m_label = m_inputBuffer;
      ExecuteAction();
      return;
    }
  }

  if (!m_timer.IsRunning())
    m_timer.Start(m_iDelay);
  else
    m_timer.Restart();
}

CPVRChannelNumber CPVRChannelNumberInputHandler::GetChannelNumber() const
{
  int iChannelNumber = 0;
  int iSubChannelNumber = 0;

  CSingleLock lock(m_mutex);

  size_t pos = m_inputBuffer.find(CPVRChannelNumber::SEPARATOR);
  if (pos != std::string::npos)
  {
    // main + sub
    if (pos != 0)
    {
      iChannelNumber = std::atoi(m_inputBuffer.substr(0, pos).c_str());
      if (pos != m_inputBuffer.size() - 1)
        iSubChannelNumber = std::atoi(m_inputBuffer.substr(pos + 1).c_str());
    }
  }
  else
  {
    // only main
    iChannelNumber = std::atoi(m_inputBuffer.c_str());
  }

  return CPVRChannelNumber(iChannelNumber, iSubChannelNumber);
}

bool CPVRChannelNumberInputHandler::HasChannelNumber() const
{
  return !m_inputBuffer.empty();
}

std::string CPVRChannelNumberInputHandler::GetChannelNumberLabel() const
{
  CSingleLock lock(m_mutex);
  return m_label;
}

} // namespace PVR
