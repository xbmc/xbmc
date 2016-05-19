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

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/AddonInterfaces.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "dialogs/GUIDialogExtendedProgressBar.h"
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

void CAddOnDialog_ExtendedProgress::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Dialogs.ExtendedProgress.New            = CAddOnDialog_ExtendedProgress::New;
  interfaces->GUI.Dialogs.ExtendedProgress.Delete         = CAddOnDialog_ExtendedProgress::Delete;
  interfaces->GUI.Dialogs.ExtendedProgress.Title          = CAddOnDialog_ExtendedProgress::Title;
  interfaces->GUI.Dialogs.ExtendedProgress.SetTitle       = CAddOnDialog_ExtendedProgress::SetTitle;
  interfaces->GUI.Dialogs.ExtendedProgress.Text           = CAddOnDialog_ExtendedProgress::Text;
  interfaces->GUI.Dialogs.ExtendedProgress.SetText        = CAddOnDialog_ExtendedProgress::SetText;
  interfaces->GUI.Dialogs.ExtendedProgress.IsFinished     = CAddOnDialog_ExtendedProgress::IsFinished;
  interfaces->GUI.Dialogs.ExtendedProgress.MarkFinished   = CAddOnDialog_ExtendedProgress::MarkFinished;
  interfaces->GUI.Dialogs.ExtendedProgress.Percentage     = CAddOnDialog_ExtendedProgress::Percentage;
  interfaces->GUI.Dialogs.ExtendedProgress.SetPercentage  = CAddOnDialog_ExtendedProgress::SetPercentage;
  interfaces->GUI.Dialogs.ExtendedProgress.SetProgress    = CAddOnDialog_ExtendedProgress::SetProgress;
}

void* CAddOnDialog_ExtendedProgress::New(void *addonData, const char *title)
{
  try
  {
    if (!addonData || !title)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    // setup the progress dialog
    CGUIDialogExtendedProgressBar* pDlgProgress = dynamic_cast<CGUIDialogExtendedProgressBar *>(g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS));
    CGUIDialogProgressBarHandle* dlgProgressHandle = pDlgProgress->GetHandle(title);
    return dlgProgressHandle;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnDialog_ExtendedProgress::Delete(void *addonData, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgressBarHandle* dlgProgressHandle = static_cast<CGUIDialogProgressBarHandle *>(handle);
    dlgProgressHandle->MarkFinished();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDialog_ExtendedProgress::Title(void *addonData, void* handle, char &title, unsigned int &iMaxStringSize)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgressBarHandle* dlgProgressHandle = static_cast<CGUIDialogProgressBarHandle *>(handle);
    strncpy(&title, dlgProgressHandle->Title().c_str(), iMaxStringSize);
    iMaxStringSize = dlgProgressHandle->Title().length();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDialog_ExtendedProgress::SetTitle(void *addonData, void* handle, const char *title)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle || !title)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgressBarHandle* dlgProgressHandle = static_cast<CGUIDialogProgressBarHandle *>(handle);
    dlgProgressHandle->SetTitle(title);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDialog_ExtendedProgress::Text(void *addonData, void* handle, char &text, unsigned int &iMaxStringSize)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgressBarHandle* dlgProgressHandle = static_cast<CGUIDialogProgressBarHandle *>(handle);
    strncpy(&text, dlgProgressHandle->Text().c_str(), iMaxStringSize);
    iMaxStringSize = dlgProgressHandle->Text().length();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDialog_ExtendedProgress::SetText(void *addonData, void* handle, const char *text)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle || !text)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgressBarHandle* dlgProgressHandle = static_cast<CGUIDialogProgressBarHandle *>(handle);
    dlgProgressHandle->SetText(text);
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnDialog_ExtendedProgress::IsFinished(void *addonData, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgressBarHandle* dlgProgressHandle = static_cast<CGUIDialogProgressBarHandle *>(handle);
    return dlgProgressHandle->IsFinished();
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnDialog_ExtendedProgress::MarkFinished(void *addonData, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgressBarHandle* dlgProgressHandle = static_cast<CGUIDialogProgressBarHandle *>(handle);
    dlgProgressHandle->MarkFinished();
  }
  HANDLE_ADDON_EXCEPTION
}

float CAddOnDialog_ExtendedProgress::Percentage(void *addonData, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgressBarHandle* dlgProgressHandle = static_cast<CGUIDialogProgressBarHandle *>(handle);
    return dlgProgressHandle->Percentage();
  }
  HANDLE_ADDON_EXCEPTION

  return 0.0f;
}

void CAddOnDialog_ExtendedProgress::SetPercentage(void *addonData, void* handle, float fPercentage)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgressBarHandle* dlgProgressHandle = static_cast<CGUIDialogProgressBarHandle *>(handle);
    dlgProgressHandle->SetPercentage(fPercentage);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDialog_ExtendedProgress::SetProgress(void *addonData, void* handle, int currentItem, int itemCount)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgressBarHandle* dlgProgressHandle = static_cast<CGUIDialogProgressBarHandle *>(handle);
    dlgProgressHandle->SetProgress(currentItem, itemCount);
  }
  HANDLE_ADDON_EXCEPTION
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
