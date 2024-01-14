/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogButtonCapture.h"

#include "ServiceBroker.h"
#include "games/controllers/ControllerIDs.h"
#include "input/actions/ActionIDs.h"
#include "input/joysticks/JoystickUtils.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/keymaps/interfaces/IKeymap.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "peripherals/Peripherals.h"
#include "utils/Variant.h"

#include <algorithm>
#include <iterator>

using namespace KODI;
using namespace GAME;

CGUIDialogButtonCapture::CGUIDialogButtonCapture() : CThread("ButtonCaptureDlg")
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

    bool bAccepted = MESSAGING::HELPERS::ShowOKDialogText(CVariant{GetDialogHeader()},
                                                          CVariant{GetDialogText()});

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
    MESSAGING::HELPERS::UpdateOKDialogText(CVariant{35013}, CVariant{GetDialogText()});
  }
}

bool CGUIDialogButtonCapture::MapPrimitive(JOYSTICK::IButtonMap* buttonMap,
                                           KEYMAP::IKeymap* keymap,
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
      const auto& actions =
          keymap->GetActions(JOYSTICK::CJoystickUtils::MakeKeyName(feature)).actions;
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
