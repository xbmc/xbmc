/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DriverReceiving.h"

#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/joysticks/interfaces/IDriverReceiver.h"

using namespace KODI;
using namespace JOYSTICK;

CDriverReceiving::CDriverReceiving(IDriverReceiver* receiver, IButtonMap* buttonMap)
  : m_receiver(receiver), m_buttonMap(buttonMap)
{
}

bool CDriverReceiving::SetRumbleState(const FeatureName& feature, float magnitude)
{
  bool bHandled = false;

  if (m_receiver != nullptr && m_buttonMap != nullptr)
  {
    CDriverPrimitive primitive;
    if (m_buttonMap->GetScalar(feature, primitive))
    {
      if (primitive.Type() == PRIMITIVE_TYPE::MOTOR)
        bHandled = m_receiver->SetMotorState(primitive.Index(), magnitude);
    }
  }

  return bHandled;
}
