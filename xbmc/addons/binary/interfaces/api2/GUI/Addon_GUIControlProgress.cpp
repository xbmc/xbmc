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

#include "Addon_GUIControlProgress.h"

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "guilib/GUIProgressControl.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_Progress::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Control.Progress.SetVisible = CAddOnControl_Progress::SetVisible;
  interfaces->GUI.Control.Progress.SetPercentage = CAddOnControl_Progress::SetPercentage;
  interfaces->GUI.Control.Progress.GetPercentage = CAddOnControl_Progress::GetPercentage;
}

void CAddOnControl_Progress::SetVisible(void *addonData, void* handle, bool visible)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Progress - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIProgressControl*>(handle)->SetVisible(visible);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Progress::SetPercentage(void *addonData, void* handle, float fPercent)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Progress - %s - invalid handler data", __FUNCTION__);

    if (handle)
      static_cast<CGUIProgressControl*>(handle)->SetPercentage(fPercent);
  }
  HANDLE_ADDON_EXCEPTION
}

float CAddOnControl_Progress::GetPercentage(void *addonData, void* handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Progress - %s - invalid handler data", __FUNCTION__);

    return static_cast<CGUIProgressControl*>(handle)->GetPercentage();
  }
  HANDLE_ADDON_EXCEPTION

  return 0.0f;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
