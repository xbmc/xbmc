/*
*      Copyright (C) 2005-2015 Team Kodi
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

#include "DialogOKHelper.h"
#include "messaging/ApplicationMessenger.h"

namespace KODI
{
namespace MESSAGING
{
namespace HELPERS
{
bool ShowOKDialogText(CVariant heading, CVariant text)
{
  DialogOKMessage options;
  options.heading = std::move(heading);
  options.text = std::move(text);
  
  if (CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_DIALOG_OK, -1, -1, static_cast<void*>(&options)) > 0)
    return true;
  return false;
}

void UpdateOKDialogText(CVariant heading, CVariant text)
{
  DialogOKMessage options;
  options.heading = std::move(heading);
  options.text = std::move(text);
  options.show = false;

  CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_DIALOG_OK, -1, -1, static_cast<void*>(&options));
}

bool ShowOKDialogLines(CVariant heading, CVariant line0, CVariant line1, CVariant line2)
{
  DialogOKMessage options;
  options.heading = std::move(heading);
  options.lines[0] = std::move(line0);
  options.lines[1] = std::move(line1);
  options.lines[2] = std::move(line2);

  if (CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_DIALOG_OK, -1, -1, static_cast<void*>(&options)) > 0)
    return true;
  return false;
}

}
}
}