/*
*      Copyright (C) 2005-2017 Team Kodi
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
*  along with Kodi; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "GUIMessageHelper.h"
#include "messaging/ApplicationMessenger.h"
#include "guilib/GUIMessage.h"


namespace KODI
{
namespace MESSAGING
{
namespace HELPERS
{
bool SendGUIMessage(CGUIMessage& message)
{
  auto result = CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_MESSAGE, 0, -1,
    static_cast<void*>(new CGUIMessage(message)));

  return result == 1;
}

bool SendGUIMessage(int message, int senderID, int destID, int param1, int param2)
{
  auto result = CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_MESSAGE, 0, -1,
    static_cast<void*>(new CGUIMessage(message, senderID, destID, param1, param2)));

  return result == 1;
}

bool SendGUIMessage(CGUIMessage& message, int window)
{
  auto result = CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_MESSAGE, window, -1,
    static_cast<void*>(new CGUIMessage(message)));

  return result == 1;
}

void PostGUIMessage(CGUIMessage& message)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_MESSAGE, 0, -1,
    static_cast<void*>(new CGUIMessage(message)));
}

void PostGUIMessage(int message, int senderID, int destID, int param1, int param2)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_MESSAGE, 0, -1,
    static_cast<void*>(new CGUIMessage(message, senderID, destID, param1, param2)));
}

void PostGUIMessage(CGUIMessage& message, int window)
{
  CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_MESSAGE, window, -1,
    static_cast<void*>(new CGUIMessage(message)));
}
}
}
}