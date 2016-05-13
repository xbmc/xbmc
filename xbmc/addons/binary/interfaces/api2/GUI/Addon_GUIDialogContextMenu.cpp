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

#include "Addon_GUIDialogContextMenu.h"

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/GUIWindowManager.h"
#include "utils/Variant.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnDialog_ContextMenu::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Dialogs.ContextMenu.Open = V2::KodiAPI::GUI::CAddOnDialog_ContextMenu::Open;
}

int CAddOnDialog_ContextMenu::Open(const char *heading, const char *entries[], unsigned int size)
{
  try
  {
    CGUIDialogContextMenu* pDialog = (CGUIDialogContextMenu*)g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);

    CContextButtons choices;
    for (unsigned int i = 0; i < size; ++i)
      choices.Add(i, entries[i]);

    return pDialog->Show(choices);
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
