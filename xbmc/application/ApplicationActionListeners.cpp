/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationActionListeners.h"

#include "input/actions/interfaces/IActionListener.h"
#include "threads/CriticalSection.h"

#include <algorithm>
#include <mutex>

CApplicationActionListeners::CApplicationActionListeners(CCriticalSection& section)
  : m_critSection(section)
{
}

void CApplicationActionListeners::RegisterActionListener(KODI::ACTION::IActionListener* listener)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it = std::find(m_actionListeners.begin(), m_actionListeners.end(), listener);
  if (it == m_actionListeners.end())
    m_actionListeners.push_back(listener);
}

void CApplicationActionListeners::UnregisterActionListener(KODI::ACTION::IActionListener* listener)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  auto it = std::find(m_actionListeners.begin(), m_actionListeners.end(), listener);
  if (it != m_actionListeners.end())
    m_actionListeners.erase(it);
}

bool CApplicationActionListeners::NotifyActionListeners(const CAction& action) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& listener : m_actionListeners)
  {
    if (listener->OnAction(action))
      return true;
  }

  return false;
}
