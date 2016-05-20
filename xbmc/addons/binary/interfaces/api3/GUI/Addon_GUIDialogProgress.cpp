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

#include "Addon_GUIDialogProgress.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/GUI/Addon_GUIDialogProgress.h"
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

void CAddOnDialog_Progress::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Dialogs.Progress.New                = V2::KodiAPI::GUI::CAddOnDialog_Progress::New;
  interfaces->GUI.Dialogs.Progress.Delete             = V2::KodiAPI::GUI::CAddOnDialog_Progress::Delete;
  interfaces->GUI.Dialogs.Progress.Open               = V2::KodiAPI::GUI::CAddOnDialog_Progress::Open;
  interfaces->GUI.Dialogs.Progress.SetHeading         = V2::KodiAPI::GUI::CAddOnDialog_Progress::SetHeading;
  interfaces->GUI.Dialogs.Progress.SetLine            = V2::KodiAPI::GUI::CAddOnDialog_Progress::SetLine;
  interfaces->GUI.Dialogs.Progress.SetCanCancel       = V2::KodiAPI::GUI::CAddOnDialog_Progress::SetCanCancel;
  interfaces->GUI.Dialogs.Progress.IsCanceled         = V2::KodiAPI::GUI::CAddOnDialog_Progress::IsCanceled;
  interfaces->GUI.Dialogs.Progress.SetPercentage      = V2::KodiAPI::GUI::CAddOnDialog_Progress::SetPercentage;
  interfaces->GUI.Dialogs.Progress.GetPercentage      = V2::KodiAPI::GUI::CAddOnDialog_Progress::GetPercentage;
  interfaces->GUI.Dialogs.Progress.ShowProgressBar    = V2::KodiAPI::GUI::CAddOnDialog_Progress::ShowProgressBar;
  interfaces->GUI.Dialogs.Progress.SetProgressMax     = V2::KodiAPI::GUI::CAddOnDialog_Progress::SetProgressMax;
  interfaces->GUI.Dialogs.Progress.SetProgressAdvance = V2::KodiAPI::GUI::CAddOnDialog_Progress::SetProgressAdvance;
  interfaces->GUI.Dialogs.Progress.Abort              = V2::KodiAPI::GUI::CAddOnDialog_Progress::Abort;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V3 */
