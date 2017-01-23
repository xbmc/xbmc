/*
 *      Copyright (C) 2016-2017 Team Kodi
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

#include "GUIDialogButtonCapture.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/joysticks/JoystickIDs.h"
#include "input/joysticks/IButtonMap.h"
#include "input/joysticks/IButtonMapCallback.h"
#include "input/joysticks/JoystickUtils.h"
#include "input/IKeymap.h"
#include "input/ActionIDs.h"
#include "messaging/helpers/DialogOKHelper.h" 
#include "peripherals/Peripherals.h"
#include "utils/Variant.h"
#include "ServiceBroker.h"

#include <algorithm>
#include <iterator>

using namespace KODI;
using namespace GAME;
using namespace KODI::MESSAGING;

CGUIDialogButtonCapture::CGUIDialogButtonCapture() :
  CThread("ButtonCaptureDlg")
{
}

std::string CGUIDialogButtonCapture::ControllerID(void) const
{
  return DEFAULT_CONTROLLER_ID;
}

void CGUIDialogButtonCapture::Show()
{
  if (!IsRunning())
  {
    InstallHooks();

    Create();

    bool bAccepted = HELPERS::ShowOKDialogText(CVariant{ GetDialogHeader() }, CVariant{ GetDialogText() });

    StopThread(false);

    m_captureEvent.Set();

    OnClose(bAccepted);

    RemoveHooks();
  }
}

void CGUIDialogButtonCapture::Process()
{
  while (!m_bStop)
  {
    m_captureEvent.Wait();

    if (m_bStop)
      break;

    //! @todo Move to rendering thread when there is a rendering thread
    HELPERS::UpdateOKDialogText(CVariant{ 35013 }, CVariant{ GetDialogText() });
  }
}

bool CGUIDialogButtonCapture::MapPrimitive(JOYSTICK::IButtonMap* buttonMap,
                                           IKeymap* keymap,
                                           const JOYSTICK::CDriverPrimitive& primitive)
{
  if (m_bStop)
    return false;

  // First check to see if driver primitive closes the dialog
  if (keymap && keymap->ControllerID() == buttonMap->ControllerID())
  {
    std::string feature;
    if (buttonMap->GetFeature(primitive, feature))
    {
      const auto &actions = keymap->GetActions(JOYSTICK::CJoystickUtils::MakeKeyName(feature));
      if (!actions.empty())
      {
        switch (actions.begin()->actionId)
        {
        case ACTION_SELECT_ITEM:
        case ACTION_NAV_BACK:
        case ACTION_PREVIOUS_MENU:
          return false;
        default:
          break;
        }
      }
    }
  }

  return MapPrimitiveInternal(buttonMap, keymap, primitive);
}

void CGUIDialogButtonCapture::InstallHooks(void)
{
  CServiceBroker::GetPeripherals().RegisterJoystickButtonMapper(this);
  CServiceBroker::GetPeripherals().RegisterObserver(this);
}

void CGUIDialogButtonCapture::RemoveHooks(void)
{
  CServiceBroker::GetPeripherals().UnregisterObserver(this);
  CServiceBroker::GetPeripherals().UnregisterJoystickButtonMapper(this);
}

void CGUIDialogButtonCapture::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessagePeripheralsChanged:
  {
    CServiceBroker::GetPeripherals().UnregisterJoystickButtonMapper(this);
    CServiceBroker::GetPeripherals().RegisterJoystickButtonMapper(this);
    break;
  }
  default:
    break;
  }
}
