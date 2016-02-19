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
#include "guilib/GUIImage.h"

#include "AddonGUIControlImage.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_Image::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{

  callbacks->GUI.Control.Image.SetVisible      = CAddOnControl_Image::SetVisible;
  callbacks->GUI.Control.Image.SetFileName     = CAddOnControl_Image::SetFileName;
  callbacks->GUI.Control.Image.SetColorDiffuse = CAddOnControl_Image::SetColorDiffuse;
}

void CAddOnControl_Image::SetVisible(void *addonData, GUIHANDLE handle, bool visible)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Image - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIImage*>(handle)->SetVisible(visible);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Image::SetFileName(void *addonData, GUIHANDLE handle, const char* strFileName, const bool useCache)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Image - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIImage*>(handle)->SetFileName(strFileName, false, useCache);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Image::SetColorDiffuse(void *addonData, GUIHANDLE handle, uint32_t colorDiffuse)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Image - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIImage*>(handle)->SetColorDiffuse(colorDiffuse);
  }
  HANDLE_ADDON_EXCEPTION
}

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
