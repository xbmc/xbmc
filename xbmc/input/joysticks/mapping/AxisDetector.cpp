/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AxisDetector.h"

#include "input/joysticks/JoystickTranslator.h"
#include "utils/log.h"

#include <cstdlib>

using namespace KODI;
using namespace JOYSTICK;

// Axis must exceed this value to be mapped
constexpr float AXIS_THRESHOLD = 0.75f;

// Delay trigger detection to handle anomalous triggers with non-zero center
constexpr unsigned int TRIGGER_DELAY_MS = 200;

CAxisDetector::CAxisDetector(CButtonMapping* buttonMapping,
                             unsigned int axisIndex,
                             const AxisConfiguration& config)
  : CPrimitiveDetector(buttonMapping), m_axisIndex(axisIndex), m_config(config)
{
}

bool CAxisDetector::OnMotion(float position)
{
  DetectType(position);

  if (m_type != AXIS_TYPE::UNKNOWN)
  {
    // Update position if this axis is an anomalous trigger
    if (m_type == AXIS_TYPE::OFFSET)
      position = (position - m_config.center) / m_config.range;

    // Reset state if position crosses zero
    if (m_state == AXIS_STATE::MAPPED)
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
        m_activatedPrimitive =
            CDriverPrimitive(m_axisIndex, m_config.center,
                             CJoystickTranslator::PositionToSemiAxisDirection(position), 1);
        m_activationTimeMs = std::chrono::steady_clock::now();
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
      auto now = std::chrono::steady_clock::now();
      auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(now - m_activationTimeMs);

      if (duration.count() < TRIGGER_DELAY_MS)
        bIgnore = true;
    }

    if (!bIgnore)
    {
      // Update driver primitive's range if we're mapping an anomalous trigger
      if (m_type == AXIS_TYPE::OFFSET)
      {
        m_activatedPrimitive =
            CDriverPrimitive(m_activatedPrimitive.Index(), m_activatedPrimitive.Center(),
                             m_activatedPrimitive.SemiAxisDirection(), m_config.range);
      }

      // Map primitive
      if (!MapPrimitive(m_activatedPrimitive))
      {
        if (m_type == AXIS_TYPE::OFFSET)
          CLog::Log(LOGDEBUG, "Mapping offset axis {} failed", m_axisIndex);
        else
          CLog::Log(LOGDEBUG, "Mapping normal axis {} failed", m_axisIndex);
      }

      m_state = AXIS_STATE::MAPPED;
    }
  }
}

void CAxisDetector::SetEmitted(const CDriverPrimitive& activePrimitive)
{
  m_state = AXIS_STATE::MAPPED;
  m_activatedPrimitive = activePrimitive;
}

void CAxisDetector::DetectType(float position)
{
  // Some platforms don't report a value until the axis is first changed.
  // Detection relies on an initial value, so this axis will be disabled until
  // the user begins button mapping again.
  if (m_config.bLateDiscovery)
    return;

  // Update range if a range of > 1 is observed
  if (std::abs(position - m_config.center) > 1.0f)
    m_config.range = 2;

  if (m_type != AXIS_TYPE::UNKNOWN)
    return;

  if (m_config.bKnown)
  {
    if (m_config.center == 0)
      m_type = AXIS_TYPE::NORMAL;
    else
      m_type = AXIS_TYPE::OFFSET;
  }

  if (m_type != AXIS_TYPE::UNKNOWN)
    return;

  if (!m_initialPositionKnown)
  {
    m_initialPositionKnown = true;
    m_initialPosition = position;
  }

  if (position != m_initialPosition)
    m_initialPositionChanged = true;

  if (m_initialPositionChanged)
  {
    // Calculate center based on initial position.
    if (m_initialPosition < -0.5f)
    {
      m_config.center = -1;
      m_type = AXIS_TYPE::OFFSET;
      CLog::Log(LOGDEBUG, "Anomalous trigger detected on axis {} with center {}", m_axisIndex,
                m_config.center);
    }
    else if (m_initialPosition > 0.5f)
    {
      m_config.center = 1;
      m_type = AXIS_TYPE::OFFSET;
      CLog::Log(LOGDEBUG, "Anomalous trigger detected on axis {} with center {}", m_axisIndex,
                m_config.center);
    }
    else
    {
      m_type = AXIS_TYPE::NORMAL;
      CLog::Log(LOGDEBUG, "Normal axis detected on axis {}", m_axisIndex);
    }
  }
}
