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
#include "input/actions/ActionIDs.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/JoystickUtils.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/joysticks/interfaces/IButtonMapper.h"
#include "input/keyboard/Key.h"
#include "input/keymaps/interfaces/IKeymap.h"
#include "peripherals/PeripheralTypes.h"
#include "peripherals/Peripherals.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/log.h"

#include <chrono>
#include <utility>
#include <vector>

using namespace KODI;
using namespace JOYSTICK;

// Guard against repeated input
constexpr unsigned int MAPPING_COOLDOWN_MS = 50;

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

  ++m_frameCount;
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

  if (m_buttonMap->IsIgnored(primitive))
  {
    bHandled = true;
  }
  else
  {
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
    else
    {
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastAction);

      CLog::Log(LOGDEBUG, "Button mapping: rapid input after {}ms dropped for profile \"{}\"",
                duration.count(), m_buttonMapper->ControllerID());
      bHandled = true;
    }
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

    bool isGcController = false;

    // Avoid showing the "capture input" dialog for the GCController driver, as
    // analog stick axes are always late due to zeroed events not always being
    // sent
#if defined(TARGET_DARWIN)
    const std::string peripheralLocation = m_buttonMap->Location();

    PERIPHERALS::CPeripherals& peripheralManager = CServiceBroker::GetPeripherals();

    const PERIPHERALS::PeripheralPtr peripheral = peripheralManager.GetByPath(peripheralLocation);

    if (peripheral &&
        peripheral->GetBusType() == PERIPHERALS::PeripheralBusType::PERIPHERAL_BUS_GCCONTROLLER)
      isGcController = true;
#endif

    if (m_frameCount >= 2 && !isGcController)
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
