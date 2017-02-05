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
#include "games/GameServices.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerFeature.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/IActionMap.h"
#include "input/joysticks/IButtonMap.h"
#include "input/joysticks/IButtonMapper.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/Key.h"
#include "threads/SystemClock.h"
#include "utils/log.h"
#include "ServiceBroker.h"

#include <algorithm>
#include <assert.h>
#include <cmath>

using namespace JOYSTICK;
using namespace XbmcThreads;

#define MAPPING_COOLDOWN_MS  50    // Guard against repeated input
#define AXIS_THRESHOLD       0.75f // Axis must exceed this value to be mapped
#define TRIGGER_DELAY_MS     200   // Delay trigger detection to handle anomalous triggers with non-zero center

// --- CButtonDetector ---------------------------------------------------------

CButtonDetector::CButtonDetector(CButtonMapping* buttonMapping, unsigned int buttonIndex) :
  m_buttonMapping(buttonMapping),
  m_buttonIndex(buttonIndex)
{
}

bool CButtonDetector::OnMotion(bool bPressed)
{
  if (bPressed)
  {
    CDriverPrimitive buttonPrimitive(PRIMITIVE_TYPE::BUTTON, m_buttonIndex);
    if (buttonPrimitive.IsValid())
    {
      return m_buttonMapping->MapPrimitive(buttonPrimitive);
    }
  }

  return false;
}

// --- CHatDetector ------------------------------------------------------------

CHatDetector::CHatDetector(CButtonMapping* buttonMapping, unsigned int hatIndex) :
  m_buttonMapping(buttonMapping),
  m_hatIndex(hatIndex)
{
}

bool CHatDetector::OnMotion(HAT_STATE state)
{
  CDriverPrimitive hatPrimitive(m_hatIndex, static_cast<HAT_DIRECTION>(state));
  if (hatPrimitive.IsValid())
  {
    m_buttonMapping->MapPrimitive(hatPrimitive);
    return true;
  }

  return false;
}

// --- CAxisDetector -----------------------------------------------------------

CAxisDetector::CAxisDetector(CButtonMapping* buttonMapping, unsigned int axisIndex, const AxisConfiguration& config) :
  m_buttonMapping(buttonMapping),
  m_axisIndex(axisIndex),
  m_config(config),
  m_state(AXIS_STATE::INACTIVE),
  m_type(AXIS_TYPE::UNKNOWN),
  m_bContinuous(false),
  m_initialPositionKnown(false),
  m_initialPosition(0.0f),
  m_initialPositionChanged(false),
  m_bDiscreteDpadMapped(false),
  m_activationTimeMs(0)
{
}

bool CAxisDetector::OnMotion(float position)
{
  if (m_type == AXIS_TYPE::UNKNOWN)
  {
    if (m_config.bKnown)
    {
      if (m_config.center == 0)
        m_type = AXIS_TYPE::NORMAL;
      else
        m_type = AXIS_TYPE::OFFSET;
    }
    else
    {
      DetectType(position);
    }
  }

  if (m_type != AXIS_TYPE::UNKNOWN)
  {
    // Update range if a range of > 1 is observed
    if (std::abs(position - m_config.center) > 1.0f)
      m_config.range = 2;

    // Update position if this axis is an anomalous trigger
    if (m_type == AXIS_TYPE::OFFSET)
      position = (position - m_config.center) / m_config.range;

    // We must observe two integer values to detect a discrete D-pad. In this
    // case, the first value is the one that should be mapped. We only need to
    // do this once.
    if (!m_bContinuous && !m_bDiscreteDpadMapped)
    {
      if (m_initialPosition != 0.0f)
        position = m_initialPosition;

      m_bDiscreteDpadMapped = true; // position must be non-zero
    }

    // Reset state if position crosses zero
    if (m_state != AXIS_STATE::INACTIVE)
    {
      SEMIAXIS_DIRECTION activatedDir = m_activatedPrimitive.SemiAxisDirection();
      SEMIAXIS_DIRECTION newDir = CJoystickTranslator::PositionToSemiAxisDirection(position);

      if (activatedDir != newDir)
        m_state = AXIS_STATE::INACTIVE;
    }

    // Check if axis has become activated
    if (m_state == AXIS_STATE::INACTIVE)
    {
      if (std::abs(position) >= AXIS_THRESHOLD)
        m_state = AXIS_STATE::ACTIVATED;

      if (m_state == AXIS_STATE::ACTIVATED)
      {
        // Range is set later for anomalous triggers
        m_activatedPrimitive = CDriverPrimitive(m_axisIndex, m_config.center, CJoystickTranslator::PositionToSemiAxisDirection(position), 1);
        m_activationTimeMs = SystemClockMillis();
      }
    }
  }

  return true;
}

void CAxisDetector::ProcessMotion()
{
  // Process newly-activated axis
  if (m_state == AXIS_STATE::ACTIVATED)
  {
    // Ignore anomalous triggers for a bit so we can detect the full range
    bool bIgnore = false;
    if (m_type == AXIS_TYPE::OFFSET)
    {
      unsigned int elapsedMs = SystemClockMillis() - m_activationTimeMs;
      if (elapsedMs < TRIGGER_DELAY_MS)
        bIgnore = true;
    }

    if (!bIgnore)
    {
      // Update driver primitive if we're mapping an anomalous trigger
      if (m_type == AXIS_TYPE::OFFSET)
        m_activatedPrimitive = CDriverPrimitive(m_axisIndex, m_config.center, m_activatedPrimitive.SemiAxisDirection(), m_config.range);

      // Map primitive
      if (!m_buttonMapping->MapPrimitive(m_activatedPrimitive))
      {
        if (!m_bContinuous)
          CLog::Log(LOGDEBUG, "Mapping discrete D-pad on axis %u failed", m_axisIndex);
        else if (m_type == AXIS_TYPE::OFFSET)
          CLog::Log(LOGDEBUG, "Mapping offset axis %u failed", m_axisIndex);
        else
          CLog::Log(LOGDEBUG, "Mapping normal axis %u failed", m_axisIndex);
      }

      m_state = AXIS_STATE::MAPPED;
    }
  }
}

void CAxisDetector::DetectType(float position)
{
  // Calculate center based on initial position.
  //
  // The idea behind this is that "initial perturbations are minimal". This
  // means that, assuming the timestep is small enough, that the position will
  // only have moved a small distance from the rest state.
  //
  // In practice, this assumption breaks for fast movements, especially on OSX
  // where events are asynchronous and occur at larger timesteps. There's
  // nothing we can do about this, so the user will have to try again with
  // slower motion.
  //
  // To differentiate discrete D-pads from anomalous triggers, we need to use a
  // second position value. If the position jumps discretely it's a discrete
  // D-pad, if it travels continuously then it's an anomalous trigger.
  //

  // Update state
  if (position != -1.0f && position != 0.0f && position != 1.0f)
    m_bContinuous = true;

  if (!m_initialPositionKnown)
  {
    m_initialPositionKnown = true;
    m_initialPosition = position;
  }

  if (!m_initialPositionChanged && position != m_initialPosition)
    m_initialPositionChanged = true;

  // Apply conditions for type detection
  if (m_bContinuous)
  {
    if (position < -0.5f)
    {
      m_config.center = -1;
      m_type = AXIS_TYPE::OFFSET;
      CLog::Log(LOGDEBUG, "Anomalous trigger detected on axis %u with center %d", m_axisIndex, m_config.center);
    }
    else if (position > 0.5f)
    {
      m_config.center = 1;
      m_type = AXIS_TYPE::OFFSET;
      CLog::Log(LOGDEBUG, "Anomalous trigger detected on axis %u with center %d", m_axisIndex, m_config.center);
    }
    else
    {
      m_type = AXIS_TYPE::NORMAL;
    }
  }
  else
  {
    // Detect a discrete D-pad when two discrete values have been observed
    if (m_initialPositionChanged)
    {
      m_type = AXIS_TYPE::NORMAL; // Axis is a discrete D-pad
      CLog::Log(LOGDEBUG, "Discrete D-pad detected on axis %u", m_axisIndex);
    }
  }
}

// --- CButtonMapping ----------------------------------------------------------

CButtonMapping::CButtonMapping(IButtonMapper* buttonMapper, IButtonMap* buttonMap, IActionMap* actionMap) :
  m_buttonMapper(buttonMapper),
  m_buttonMap(buttonMap),
  m_actionMap(actionMap),
  m_lastAction(0)
{
  assert(m_buttonMapper != nullptr);
  assert(m_buttonMap != nullptr);

  // Make sure axes mapped to Select are centered before they can be mapped.
  // This ensures that they are not immediately mapped to the first button.
  if (m_actionMap && m_actionMap->ControllerID() == m_buttonMap->ControllerID())
  {
    using namespace GAME;

    CGameServices& gameServices = CServiceBroker::GetGameServices();
    ControllerPtr controller = gameServices.GetController(m_actionMap->ControllerID());

    const auto& features = controller->Layout().Features();
    for (const auto& feature : features)
    {
      if (m_actionMap->GetActionID(feature.Name()) != ACTION_SELECT_ITEM)
        continue;

      CDriverPrimitive primitive;
      if (!m_buttonMap->GetScalar(feature.Name(), primitive))
        continue;

      if (primitive.Type() != PRIMITIVE_TYPE::SEMIAXIS)
        continue;

      // Set initial config, as detection will fail because axis is already activated
      AxisConfiguration axisConfig;
      axisConfig.bKnown = true;
      axisConfig.center = primitive.Center();
      axisConfig.range = primitive.Range();

      GetAxis(primitive.Index(), axisConfig).SetEmitted();
    }
  }
}

bool CButtonMapping::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  return GetButton(buttonIndex).OnMotion(bPressed);
}

bool CButtonMapping::OnHatMotion(unsigned int hatIndex, HAT_STATE state)
{
  return GetHat(hatIndex).OnMotion(state);
}

bool CButtonMapping::OnAxisMotion(unsigned int axisIndex, float position)
{
  return GetAxis(axisIndex).OnMotion(position);
}

void CButtonMapping::ProcessAxisMotions(void)
{
  for (auto& axis : m_axes)
    axis.second.ProcessMotion();

  m_buttonMapper->OnEventFrame(m_buttonMap, IsMapping());
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

bool CButtonMapping::IsMapping() const
{
  for (auto itAxis : m_axes)
  {
    if (itAxis.second.IsMapping())
      return true;
  }

  return false;
}

CButtonDetector& CButtonMapping::GetButton(unsigned int buttonIndex)
{
  auto itButton = m_buttons.find(buttonIndex);

  if (itButton == m_buttons.end())
  {
    m_buttons.insert(std::make_pair(buttonIndex, CButtonDetector(this, buttonIndex)));
    itButton = m_buttons.find(buttonIndex);
  }

  return itButton->second;
}

CHatDetector& CButtonMapping::GetHat(unsigned int hatIndex)
{
  auto itHat = m_hats.find(hatIndex);

  if (itHat == m_hats.end())
  {
    m_hats.insert(std::make_pair(hatIndex, CHatDetector(this, hatIndex)));
    itHat = m_hats.find(hatIndex);
  }

  return itHat->second;
}

CAxisDetector& CButtonMapping::GetAxis(unsigned int axisIndex,
                                       const AxisConfiguration& initialConfig /* = AxisConfiguration() */)
{
  auto itAxis = m_axes.find(axisIndex);

  if (itAxis == m_axes.end())
  {
    m_axes.insert(std::make_pair(axisIndex, CAxisDetector(this, axisIndex, initialConfig)));
    itAxis = m_axes.find(axisIndex);
  }

  return itAxis->second;
}
