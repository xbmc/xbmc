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

#include "Addon_GUIDialogSelect.h"

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "dialogs/GUIDialogSelect.h"
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

void CAddOnDialog_Select::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Dialogs.Select.Open = V2::KodiAPI::GUI::CAddOnDialog_Select::Open;
}

int CAddOnDialog_Select::Open(const char *heading, const char *entries[], unsigned int size, int selected, bool autoclose)
{
  try
  {
    CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDialog->Reset();
    pDialog->SetHeading(CVariant{heading});

    for (unsigned int i = 0; i < size; ++i)
      pDialog->Add(entries[i]);

    if (selected > 0)
      pDialog->SetSelected(selected);
    if (autoclose > 0)
      pDialog->SetAutoClose(autoclose);

    pDialog->Open();
    return pDialog->GetSelectedItem();
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
