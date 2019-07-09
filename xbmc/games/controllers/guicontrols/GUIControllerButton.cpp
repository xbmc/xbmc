/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIControllerButton.h"

#include "games/controllers/windows/GUIControllerDefines.h"

using namespace KODI;
using namespace GAME;

CGUIControllerButton::CGUIControllerButton(const CGUIButtonControl& buttonControl, const std::string& label, unsigned int index) :
  CGUIButtonControl(buttonControl)
{
  // Initialize CGUIButtonControl
  SetLabel(label);
  SetID(CONTROL_CONTROLLER_BUTTONS_START + index);
  SetVisible(true);
  AllocResources();
}
