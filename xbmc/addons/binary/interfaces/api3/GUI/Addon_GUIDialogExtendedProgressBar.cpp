/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon_GUIDialogExtendedProgressBar.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/GUI/Addon_GUIDialogExtendedProgressBar.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

using namespace ADDON;

namespace V3
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnDialog_ExtendedProgress::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Dialogs.ExtendedProgress.New            = V2::KodiAPI::GUI::CAddOnDialog_ExtendedProgress::New;
  interfaces->GUI.Dialogs.ExtendedProgress.Delete         = V2::KodiAPI::GUI::CAddOnDialog_ExtendedProgress::Delete;
  interfaces->GUI.Dialogs.ExtendedProgress.Title          = V2::KodiAPI::GUI::CAddOnDialog_ExtendedProgress::Title;
  interfaces->GUI.Dialogs.ExtendedProgress.SetTitle       = V2::KodiAPI::GUI::CAddOnDialog_ExtendedProgress::SetTitle;
  interfaces->GUI.Dialogs.ExtendedProgress.Text           = V2::KodiAPI::GUI::CAddOnDialog_ExtendedProgress::Text;
  interfaces->GUI.Dialogs.ExtendedProgress.SetText        = V2::KodiAPI::GUI::CAddOnDialog_ExtendedProgress::SetText;
  interfaces->GUI.Dialogs.ExtendedProgress.IsFinished     = V2::KodiAPI::GUI::CAddOnDialog_ExtendedProgress::IsFinished;
  interfaces->GUI.Dialogs.ExtendedProgress.MarkFinished   = V2::KodiAPI::GUI::CAddOnDialog_ExtendedProgress::MarkFinished;
  interfaces->GUI.Dialogs.ExtendedProgress.Percentage     = V2::KodiAPI::GUI::CAddOnDialog_ExtendedProgress::Percentage;
  interfaces->GUI.Dialogs.ExtendedProgress.SetPercentage  = V2::KodiAPI::GUI::CAddOnDialog_ExtendedProgress::SetPercentage;
  interfaces->GUI.Dialogs.ExtendedProgress.SetProgress    = V2::KodiAPI::GUI::CAddOnDialog_ExtendedProgress::SetProgress;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V3 */
