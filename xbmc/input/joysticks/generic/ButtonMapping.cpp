/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ButtonMapping.h"

#include "ServiceBroker.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerManager.h"
#include "games/controllers/input/PhysicalFeature.h"
#include "input/InputTranslator.h"
#include "input/actions/ActionIDs.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/joysticks/JoystickUtils.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/joysticks/interfaces/IButtonMapper.h"
#include "input/keyboard/Key.h"
#include "input/keymaps/interfaces/IKeymap.h"
#include "utils/log.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <memory>

using namespace KODI;
using namespace JOYSTICK;

#define MAPPING_COOLDOWN_MS 50 // Guard against repeated input
#define AXIS_THRESHOLD 0.75f // Axis must exceed this value to be mapped
#define TRIGGER_DELAY_MS \
  200 // Delay trigger detection to handle anomalous triggers with non-zero center

// --- CPrimitiveDetector ------------------------------------------------------

CPrimitiveDetector::CPrimitiveDetector(CButtonMapping* buttonMapping)
  : m_buttonMapping(buttonMapping)
{
}

bool CPrimitiveDetector::MapPrimitive(const CDriverPrimitive& primitive)
{
  if (primitive.IsValid())
    return m_buttonMapping->MapPrimitive(primitive);

  return false;
}

// --- CButtonDetector ---------------------------------------------------------

CButtonDetector::CButtonDetector(CButtonMapping* buttonMapping, unsigned int buttonIndex)
  : CPrimitiveDetector(buttonMapping), m_buttonIndex(buttonIndex)
{
}

bool CButtonDetector::OnMotion(bool bPressed)
{
  if (bPressed)
    return MapPrimitive(CDriverPrimitive(PRIMITIVE_TYPE::BUTTON, m_buttonIndex));

  return false;
}

// --- CHatDetector ------------------------------------------------------------

CHatDetector::CHatDetector(CButtonMapping* buttonMapping, unsigned int hatIndex)
  : CPrimitiveDetector(buttonMapping), m_hatIndex(hatIndex)
{
}

bool CHatDetector::OnMotion(HAT_STATE state)
{
  return MapPrimitive(CDriverPrimitive(m_hatIndex, static_cast<HAT_DIRECTION>(state)));
}

// --- CAxisDetector -----------------------------------------------------------

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

// --- CKeyDetector ---------------------------------------------------------

CKeyDetector::CKeyDetector(CButtonMapping* buttonMapping, XBMCKey keycode)
  : CPrimitiveDetector(buttonMapping), m_keycode(keycode)
{
}

bool CKeyDetector::OnMotion(bool bPressed)
{
  if (bPressed)
    return MapPrimitive(CDriverPrimitive(m_keycode));

  return false;
}

// --- CMouseButtonDetector ----------------------------------------------------

CMouseButtonDetector::CMouseButtonDetector(CButtonMapping* buttonMapping,
                                           MOUSE::BUTTON_ID buttonIndex)
  : CPrimitiveDetector(buttonMapping), m_buttonIndex(buttonIndex)
{
}

bool CMouseButtonDetector::OnMotion(bool bPressed)
{
  if (bPressed)
    return MapPrimitive(CDriverPrimitive(m_buttonIndex));

  return false;
}

// --- CPointerDetector --------------------------------------------------------

CPointerDetector::CPointerDetector(CButtonMapping* buttonMapping)
  : CPrimitiveDetector(buttonMapping)
{
}

bool CPointerDetector::OnMotion(int x, int y)
{
  if (!m_bStarted)
  {
    m_bStarted = true;
    m_startX = x;
    m_startY = y;
    m_frameCount = 0;
  }

  if (m_frameCount++ >= MIN_FRAME_COUNT)
  {
    int dx = x - m_startX;
    int dy = y - m_startY;

    INPUT::INTERCARDINAL_DIRECTION dir = GetPointerDirection(dx, dy);

    CDriverPrimitive primitive(static_cast<RELATIVE_POINTER_DIRECTION>(dir));
    if (primitive.IsValid())
    {
      if (MapPrimitive(primitive))
        m_bStarted = false;
    }
  }

  return true;
}

KODI::INPUT::INTERCARDINAL_DIRECTION CPointerDetector::GetPointerDirection(int x, int y)
{
  using namespace INPUT;

  // Translate from left-handed coordinate system to right-handed coordinate system
  y *= -1;

  return CInputTranslator::VectorToIntercardinalDirection(static_cast<float>(x),
                                                          static_cast<float>(y));
}

// --- CButtonMapping ----------------------------------------------------------

CButtonMapping::CButtonMapping(IButtonMapper* buttonMapper,
                               IButtonMap* buttonMap,
                               KEYMAP::IKeymap* keymap)
  : m_buttonMapper(buttonMapper), m_buttonMap(buttonMap), m_keymap(keymap)
{
  assert(m_buttonMapper != nullptr);
  assert(m_buttonMap != nullptr);

  // Make sure axes mapped to Select are centered before they can be mapped.
  // This ensures that they are not immediately mapped to the first button.
  if (m_keymap)
  {
    using namespace GAME;

    CControllerManager& controllerManager = CServiceBroker::GetGameControllerManager();
    ControllerPtr controller = controllerManager.GetController(m_keymap->ControllerID());

    const auto& features = controller->Features();
    for (const auto& feature : features)
    {
      bool bIsSelectAction = false;

      const auto& actions =
          m_keymap->GetActions(CJoystickUtils::MakeKeyName(feature.Name())).actions;
      if (!actions.empty() && actions.begin()->actionId == ACTION_SELECT_ITEM)
        bIsSelectAction = true;

      if (!bIsSelectAction)
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

      GetAxis(primitive.Index(), static_cast<float>(primitive.Center()), axisConfig)
          .SetEmitted(primitive);
    }
  }
}

bool CButtonMapping::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  if (!m_buttonMapper->AcceptsPrimitive(PRIMITIVE_TYPE::BUTTON))
    return false;

  return GetButton(buttonIndex).OnMotion(bPressed);
}

bool CButtonMapping::OnHatMotion(unsigned int hatIndex, HAT_STATE state)
{
  if (!m_buttonMapper->AcceptsPrimitive(PRIMITIVE_TYPE::HAT))
    return false;

  return GetHat(hatIndex).OnMotion(state);
}

bool CButtonMapping::OnAxisMotion(unsigned int axisIndex,
                                  float position,
                                  int center,
                                  unsigned int range)
{
  if (!m_buttonMapper->AcceptsPrimitive(PRIMITIVE_TYPE::SEMIAXIS))
    return false;

  return GetAxis(axisIndex, position).OnMotion(position);
}

void CButtonMapping::OnInputFrame(void)
{
  for (auto& axis : m_axes)
    axis.second.ProcessMotion();

  m_buttonMapper->OnEventFrame(m_buttonMap, IsMapping());

  m_frameCount++;
}

bool CButtonMapping::OnKeyPress(const CKey& key)
{
  if (!m_buttonMapper->AcceptsPrimitive(PRIMITIVE_TYPE::KEY))
    return false;

  return GetKey(static_cast<XBMCKey>(key.GetKeycode())).OnMotion(true);
}

bool CButtonMapping::OnPosition(int x, int y)
{
  if (!m_buttonMapper->AcceptsPrimitive(PRIMITIVE_TYPE::RELATIVE_POINTER))
    return false;

  return GetPointer().OnMotion(x, y);
}

bool CButtonMapping::OnButtonPress(MOUSE::BUTTON_ID button)
{
  if (!m_buttonMapper->AcceptsPrimitive(PRIMITIVE_TYPE::MOUSE_BUTTON))
    return false;

  return GetMouseButton(button).OnMotion(true);
}

void CButtonMapping::OnButtonRelease(MOUSE::BUTTON_ID button)
{
  if (!m_buttonMapper->AcceptsPrimitive(PRIMITIVE_TYPE::MOUSE_BUTTON))
    return;

  GetMouseButton(button).OnMotion(false);
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

  auto now = std::chrono::steady_clock::now();

  bool bTimeoutElapsed = true;

  if (m_buttonMapper->NeedsCooldown())
    bTimeoutElapsed = (now >= m_lastAction + std::chrono::milliseconds(MAPPING_COOLDOWN_MS));

  if (bTimeoutElapsed)
  {
    bHandled = m_buttonMapper->MapPrimitive(m_buttonMap, m_keymap, primitive);

    if (bHandled)
      m_lastAction = std::chrono::steady_clock::now();
  }
  else if (m_buttonMap->IsIgnored(primitive))
  {
    bHandled = true;
  }
  else
  {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastAction);

    CLog::Log(LOGDEBUG, "Button mapping: rapid input after {}ms dropped for profile \"{}\"",
              duration.count(), m_buttonMapper->ControllerID());
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

CAxisDetector& CButtonMapping::GetAxis(
    unsigned int axisIndex,
    float position,
    const AxisConfiguration& initialConfig /* = AxisConfiguration() */)
{
  auto itAxis = m_axes.find(axisIndex);

  if (itAxis == m_axes.end())
  {
    AxisConfiguration config(initialConfig);

    if (m_frameCount >= 2)
    {
      config.bLateDiscovery = true;
      OnLateDiscovery(axisIndex);
    }

    // Report axis
    CLog::Log(LOGDEBUG, "Axis {} discovered at position {:.4f} after {} frames", axisIndex,
              position, static_cast<unsigned long>(m_frameCount));

    m_axes.insert(std::make_pair(axisIndex, CAxisDetector(this, axisIndex, config)));
    itAxis = m_axes.find(axisIndex);
  }

  return itAxis->second;
}

CKeyDetector& CButtonMapping::GetKey(XBMCKey keycode)
{
  auto itKey = m_keys.find(keycode);

  if (itKey == m_keys.end())
  {
    m_keys.insert(std::make_pair(keycode, CKeyDetector(this, keycode)));
    itKey = m_keys.find(keycode);
  }

  return itKey->second;
}

CMouseButtonDetector& CButtonMapping::GetMouseButton(MOUSE::BUTTON_ID buttonIndex)
{
  auto itButton = m_mouseButtons.find(buttonIndex);

  if (itButton == m_mouseButtons.end())
  {
    m_mouseButtons.insert(std::make_pair(buttonIndex, CMouseButtonDetector(this, buttonIndex)));
    itButton = m_mouseButtons.find(buttonIndex);
  }

  return itButton->second;
}

CPointerDetector& CButtonMapping::GetPointer()
{
  if (!m_pointer)
    m_pointer = std::make_unique<CPointerDetector>(this);

  return *m_pointer;
}

void CButtonMapping::OnLateDiscovery(unsigned int axisIndex)
{
  m_buttonMapper->OnLateAxis(m_buttonMap, axisIndex);
}
