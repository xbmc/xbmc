/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerActivity.h"

using namespace KODI;
using namespace GAME;

#include <algorithm>
#include <cstdlib>

void CControllerActivity::OnButtonPress(bool pressed)
{
  if (pressed)
    m_currentActivation = 1.0f;
}

void CControllerActivity::OnButtonMotion(float magnitude)
{
  m_currentActivation = std::max(magnitude, m_currentActivation);
}

void CControllerActivity::OnAnalogStickMotion(float x, float y)
{
  m_currentActivation = std::max(std::abs(x), m_currentActivation);
  m_currentActivation = std::max(std::abs(y), m_currentActivation);
}

void CControllerActivity::OnWheelMotion(float position)
{
  m_currentActivation = std::max(std::abs(position), m_currentActivation);
}

void CControllerActivity::OnThrottleMotion(float position)
{
  m_currentActivation = std::max(std::abs(position), m_currentActivation);
}

void CControllerActivity::OnInputFrame()
{
  m_lastActivation = m_currentActivation;
  m_currentActivation = 0.0f;
}
