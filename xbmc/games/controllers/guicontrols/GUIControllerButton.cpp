/*
 *      Copyright (C) 2014-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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
