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

#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"

#include "PVRChannelNumberInputHandler.h"

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
  m_digits.clear();
  m_strChannel.erase();
}

void CPVRChannelNumberInputHandler::AppendChannelNumberDigit(int iDigit)
{
  if (iDigit < 0 || iDigit > 9)
    return;

  CSingleLock lock(m_mutex);

  if (m_digits.size() == (size_t)m_iMaxDigits)
    m_digits.pop_front();

  m_digits.emplace_back(iDigit);

  // recalc channel string
  m_strChannel.erase();
  if (m_digits.size() != m_iMaxDigits || GetChannelNumber() > 0)
  {
    for (int digit : m_digits)
      m_strChannel.append(StringUtils::Format("%d", digit));
  }

  if (!m_timer.IsRunning())
    m_timer.Start(m_iDelay);
  else
    m_timer.Restart();
}

int CPVRChannelNumberInputHandler::GetChannelNumber() const
{
  int iNumber = 0;
  int iDigitMultiplier = 1;

  for (std::deque<int>::const_reverse_iterator it = m_digits.rbegin(); it != m_digits.rend(); ++it)
  {
    iNumber += (*it * iDigitMultiplier);
    iDigitMultiplier *= 10;
  }

  return iNumber;
}

std::string CPVRChannelNumberInputHandler::GetChannelNumberAsString() const
{
  CSingleLock lock(m_mutex);
  return m_strChannel;
}

} // namespace PVR
