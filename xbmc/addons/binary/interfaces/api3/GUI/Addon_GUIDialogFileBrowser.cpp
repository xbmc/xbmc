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

#include "Addon_GUIDialogFileBrowser.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/GUI/Addon_GUIDialogFileBrowser.h"
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

void CAddOnDialog_FileBrowser::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetDirectory    = V2::KodiAPI::GUI::CAddOnDialog_FileBrowser::ShowAndGetDirectory;
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetFile         = V2::KodiAPI::GUI::CAddOnDialog_FileBrowser::ShowAndGetFile;
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetFileFromDir  = V2::KodiAPI::GUI::CAddOnDialog_FileBrowser::ShowAndGetFileFromDir;
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetFileList     = V2::KodiAPI::GUI::CAddOnDialog_FileBrowser::ShowAndGetFileList;
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetSource       = V2::KodiAPI::GUI::CAddOnDialog_FileBrowser::ShowAndGetSource;
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetImage        = V2::KodiAPI::GUI::CAddOnDialog_FileBrowser::ShowAndGetImage;
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetImageList    = V2::KodiAPI::GUI::CAddOnDialog_FileBrowser::ShowAndGetImageList;
  interfaces->GUI.Dialogs.FileBrowser.ClearList              = V2::KodiAPI::GUI::CAddOnDialog_FileBrowser::ClearList;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V3 */
