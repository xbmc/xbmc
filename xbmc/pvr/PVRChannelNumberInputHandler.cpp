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
  // call the overridden worker method
  OnInputDone();

  CSingleLock lock(m_mutex);
  m_inputBuffer.erase();
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
    m_inputBuffer.erase(m_inputBuffer.begin());

  m_inputBuffer.append(&cCharacter, 1);

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
      if (pos != m_inputBuffer.back())
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

std::string CPVRChannelNumberInputHandler::GetChannelNumberAsString() const
{
  CSingleLock lock(m_mutex);
  return m_inputBuffer;
}

} // namespace PVR
