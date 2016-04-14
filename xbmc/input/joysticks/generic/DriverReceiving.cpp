/*
 *      Copyright (C) 2016 Team Kodi
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

#include "DriverReceiving.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/IButtonMap.h"
#include "input/joysticks/IDriverReceiver.h"

using namespace JOYSTICK;

CDriverReceiving::CDriverReceiving(IDriverReceiver* receiver, IButtonMap* buttonMap)
 : m_receiver(receiver),
   m_buttonMap(buttonMap)
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
