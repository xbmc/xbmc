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

#include "DialogHelper.h"
#include "messaging/ApplicationMessenger.h"

#include <utility>
#include <cassert>

namespace KODI
{
namespace MESSAGING
{
namespace HELPERS
{
DialogResponse ShowYesNoDialogText(CVariant heading, CVariant text, CVariant noLabel, CVariant yesLabel, uint32_t autoCloseTimeout)
{
  DialogYesNoMessage options;
  options.heading = std::move(heading);
  options.text = std::move(text);
  options.noLabel = std::move(noLabel);
  options.yesLabel = std::move(yesLabel);
  options.autoclose = autoCloseTimeout;
  
  switch (CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_DIALOG_YESNO, -1, -1, static_cast<void*>(&options)))
  {
  case -1:
    return DialogResponse::CANCELLED;
  case 0:
    return DialogResponse::NO;
  case 1:
    return DialogResponse::YES;
  default:
    //If we get here someone changed the return values without updating this code
    assert(false);
  }
  //This is unreachable code but we need to return something to suppress warnings about
  //no return
  return DialogResponse::CANCELLED;
}

DialogResponse ShowYesNoDialogLines(CVariant heading, CVariant line0, CVariant line1, CVariant line2, CVariant noLabel, CVariant yesLabel, uint32_t autoCloseTimeout)
{
  DialogYesNoMessage options;
  options.heading = std::move(heading);
  options.lines[0] = std::move(line0);
  options.lines[1] = std::move(line1);
  options.lines[2] = std::move(line2);
  options.noLabel = std::move(noLabel);
  options.yesLabel = std::move(yesLabel);
  options.autoclose = autoCloseTimeout;

  switch (CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_DIALOG_YESNO, -1, -1, static_cast<void*>(&options)))
  {
  case -1:
    return DialogResponse::CANCELLED;
  case 0:
    return DialogResponse::NO;
  case 1:
    return DialogResponse::YES;
  default:
    //If we get here someone changed the return values without updating this code
    assert(false);
  }
  //This is unreachable code but we need to return something to suppress warnings about
  //no return
  return DialogResponse::CANCELLED;
}

}
}
}