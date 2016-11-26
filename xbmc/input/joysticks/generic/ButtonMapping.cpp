/*
 *      Copyright (C) 2014-2016 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ButtonMapping.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/IButtonMap.h"
#include "input/joysticks/IButtonMapper.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/joysticks/JoystickUtils.h"
#include "threads/SystemClock.h"
#include "utils/log.h"

#include <algorithm>
#include <assert.h>
#include <cmath>

using namespace JOYSTICK;
using namespace XbmcThreads;

#define MAPPING_COOLDOWN_MS  50    // Guard against repeated input
#define AXIS_THRESHOLD       0.75f // Axis must exceed this value to be mapped

CButtonMapping::CButtonMapping(IButtonMapper* buttonMapper, IButtonMap* buttonMap, IActionMap* actionMap)
  : m_buttonMapper(buttonMapper),
    m_buttonMap(buttonMap),
    m_actionMap(actionMap),
    m_lastAction(0)
{
  assert(m_buttonMapper != NULL);
  assert(m_buttonMap != NULL);
}

bool CButtonMapping::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  if (bPressed)
  {
    CDriverPrimitive buttonPrimitive(PRIMITIVE_TYPE::BUTTON, buttonIndex);
    if (buttonPrimitive.IsValid())
    {
      return MapPrimitive(buttonPrimitive);
    }
  }

  return false;
}

bool CButtonMapping::OnHatMotion(unsigned int hatIndex, HAT_STATE state)
{
  CDriverPrimitive hatPrimitive(hatIndex, static_cast<HAT_DIRECTION>(state));
  if (hatPrimitive.IsValid())
  {
    MapPrimitive(hatPrimitive);
    return true;
  }

  return false;
}

bool CButtonMapping::OnAxisMotion(unsigned int axisIndex, float position)
{
  const bool bInMotion = (position != 0.0f);
  const bool bIsActive = (std::abs(position) >= AXIS_THRESHOLD);

  if (!bInMotion)
  {
    CDriverPrimitive axis(axisIndex, SEMIAXIS_DIRECTION::POSITIVE);
    CDriverPrimitive oppositeAxis(axisIndex, SEMIAXIS_DIRECTION::NEGATIVE);

    OnMotionless(axis);
    OnMotionless(oppositeAxis);

    Deactivate(axis);
    Deactivate(oppositeAxis);
  }
  else
  {
    SEMIAXIS_DIRECTION dir = CJoystickTranslator::PositionToSemiAxisDirection(position);

    CDriverPrimitive axis(axisIndex, dir);
    CDriverPrimitive oppositeAxis(axisIndex, dir * -1);

    // OnMotion() occurs within ProcessAxisMotions()
    OnMotionless(oppositeAxis);

    if (bIsActive)
      Activate(axis);
    else
      Deactivate(axis);
    Deactivate(oppositeAxis);
  }

  return true;
}

void CButtonMapping::ProcessAxisMotions(void)
{
  // Process newly-activated axes
  for (std::vector<ActivatedAxis>::iterator it = m_activatedAxes.begin(); it != m_activatedAxes.end(); ++it)
  {
    ActivatedAxis& semiaxis = *it;

    // Only emit once
    if (!semiaxis.bEmitted)
    {
      semiaxis.bEmitted = true;
      if (MapPrimitive(semiaxis.driverPrimitive))
        OnMotion(semiaxis.driverPrimitive);
    }
  }

  m_buttonMapper->OnEventFrame(m_buttonMap, IsMoving());
}

void CButtonMapping::SaveButtonMap()
{
  m_buttonMap->SaveButtonMap();
}

void CButtonMapping::ResetIgnoredPrimitives()
{
  std::vector<CDriverPrimitive> empty;
  m_buttonMap->SetIgnoredPrimitives(empty);
}

void CButtonMapping::RevertButtonMap()
{
  m_buttonMap->RevertButtonMap();
}

bool CButtonMapping::MapPrimitive(const CDriverPrimitive& primitive)
{
  bool bHandled = false;

  const unsigned int now = SystemClockMillis();

  bool bTimeoutElapsed = true;

  if (m_buttonMapper->NeedsCooldown())
    bTimeoutElapsed = (now >= m_lastAction + MAPPING_COOLDOWN_MS);

  if (bTimeoutElapsed)
  {
    bHandled = m_buttonMapper->MapPrimitive(m_buttonMap, m_actionMap, primitive);

    if (bHandled)
      m_lastAction = SystemClockMillis();
  }
  else if (m_buttonMap->IsIgnored(primitive))
  {
    bHandled = true;
  }
  else
  {
    const unsigned int elapsed = now - m_lastAction;
    CLog::Log(LOGDEBUG, "Button mapping: rapid input after %ums dropped for profile \"%s\"",
              elapsed, m_buttonMapper->ControllerID().c_str());
    bHandled = true;
  }

  return bHandled;
}

void CButtonMapping::OnMotion(const CDriverPrimitive& semiaxis)
{
  if (std::find(m_movingAxes.begin(), m_movingAxes.end(), semiaxis) == m_movingAxes.end())
    m_movingAxes.push_back(semiaxis);
}

void CButtonMapping::OnMotionless(const CDriverPrimitive& semiaxis)
{
  m_movingAxes.erase(std::remove(m_movingAxes.begin(), m_movingAxes.end(), semiaxis), m_movingAxes.end());
}

bool CButtonMapping::IsMoving() const
{
  return !m_movingAxes.empty();
}

void CButtonMapping::Activate(const CDriverPrimitive& semiaxis)
{
  if (!IsActive(semiaxis))
    m_activatedAxes.push_back(ActivatedAxis{semiaxis});
}

void CButtonMapping::Deactivate(const CDriverPrimitive& semiaxis)
{
  m_activatedAxes.erase(std::remove_if(m_activatedAxes.begin(), m_activatedAxes.end(),
    [&semiaxis](const ActivatedAxis& axis)
    {
      return semiaxis == axis.driverPrimitive;
    }), m_activatedAxes.end());
}

bool CButtonMapping::IsActive(const CDriverPrimitive& semiaxis) const
{
  return std::find_if(m_activatedAxes.begin(), m_activatedAxes.end(),
    [&semiaxis](const ActivatedAxis& axis)
    {
      return semiaxis == axis.driverPrimitive;
    }) != m_activatedAxes.end();
}
