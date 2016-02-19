/*
 *      Copyright (C) 2015 Team KODI
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

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/callbacks/AddonCallbacks.h"
#include "addons/binary/callbacks/api2/AddonCallbacksBase.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "guilib/GUIWindowManager.h"

#include "AddonGUIDialogTextViewer.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnDialog_TextViewer::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->GUI.Dialogs.TextViewer.Open = CAddOnDialog_TextViewer::Open;
}

void CAddOnDialog_TextViewer::Open(const char *heading, const char *text)
{
  try
  {
    CGUIDialogTextViewer* pDialog = dynamic_cast<CGUIDialogTextViewer *>(g_windowManager.GetWindow(WINDOW_DIALOG_TEXT_VIEWER));
    pDialog->SetHeading(heading);
    pDialog->SetText(text);
    pDialog->Open();
  }
  HANDLE_ADDON_EXCEPTION
}

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
