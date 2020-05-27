/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "KeymapActionMap.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/InputManager.h"
#include "input/Key.h"
#include "input/actions/Action.h"

using namespace KODI;
using namespace KEYBOARD;

unsigned int CKeymapActionMap::GetActionID(const CKey& key)
{
  CAction action = CServiceBroker::GetInputManager().GetAction(
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog(), key);
  return action.GetID();
}
