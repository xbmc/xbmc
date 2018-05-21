/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
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

#include "GUIDialogSubMenu.h"
#include "guilib/GUIMessage.h"

CGUIDialogSubMenu::CGUIDialogSubMenu(int id, const std::string &xmlFile)
    : CGUIDialog(id, xmlFile.c_str())
{
}

CGUIDialogSubMenu::~CGUIDialogSubMenu(void) = default;

bool CGUIDialogSubMenu::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    // someone has been clicked - deinit...
    CGUIDialog::OnMessage(message);
    Close();
    return true;
  }
  return CGUIDialog::OnMessage(message);
}
