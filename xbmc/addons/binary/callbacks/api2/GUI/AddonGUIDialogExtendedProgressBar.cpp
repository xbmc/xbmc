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
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

#include "AddonGUIDialogExtendedProgressBar.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnDialog_ExtendedProgress::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->GUI.Dialogs.ExtendedProgress.New            = CAddOnDialog_ExtendedProgress::New;
  callbacks->GUI.Dialogs.ExtendedProgress.Delete         = CAddOnDialog_ExtendedProgress::Delete;
  callbacks->GUI.Dialogs.ExtendedProgress.Title          = CAddOnDialog_ExtendedProgress::Title;
  callbacks->GUI.Dialogs.ExtendedProgress.SetTitle       = CAddOnDialog_ExtendedProgress::SetTitle;
  callbacks->GUI.Dialogs.ExtendedProgress.Text           = CAddOnDialog_ExtendedProgress::Text;
  callbacks->GUI.Dialogs.ExtendedProgress.SetText        = CAddOnDialog_ExtendedProgress::SetText;
  callbacks->GUI.Dialogs.ExtendedProgress.IsFinished     = CAddOnDialog_ExtendedProgress::IsFinished;
  callbacks->GUI.Dialogs.ExtendedProgress.MarkFinished   = CAddOnDialog_ExtendedProgress::MarkFinished;
  callbacks->GUI.Dialogs.ExtendedProgress.Percentage     = CAddOnDialog_ExtendedProgress::Percentage;
  callbacks->GUI.Dialogs.ExtendedProgress.SetPercentage  = CAddOnDialog_ExtendedProgress::SetPercentage;
  callbacks->GUI.Dialogs.ExtendedProgress.SetProgress    = CAddOnDialog_ExtendedProgress::SetProgress;
}

GUIHANDLE CAddOnDialog_ExtendedProgress::New(void *addonData, const char *title)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper || !title)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    // setup the progress dialog
    CGUIDialogExtendedProgressBar* pDlgProgress = dynamic_cast<CGUIDialogExtendedProgressBar *>(g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS));
    CGUIDialogProgressBarHandle* dlgProgressHandle = pDlgProgress->GetHandle(title);
    return dlgProgressHandle;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnDialog_ExtendedProgress::Delete(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_ExtendedProgress::Title(void *addonData, GUIHANDLE handle, char &title, unsigned int &iMaxStringSize)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_ExtendedProgress::SetTitle(void *addonData, GUIHANDLE handle, const char *title)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle || !title)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_ExtendedProgress::Text(void *addonData, GUIHANDLE handle, char &text, unsigned int &iMaxStringSize)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_ExtendedProgress::SetText(void *addonData, GUIHANDLE handle, const char *text)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle || !text)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
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

bool CAddOnDialog_ExtendedProgress::IsFinished(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_ExtendedProgress::MarkFinished(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
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

float CAddOnDialog_ExtendedProgress::Percentage(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_ExtendedProgress::SetPercentage(void *addonData, GUIHANDLE handle, float fPercentage)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_ExtendedProgress::SetProgress(void *addonData, GUIHANDLE handle, int currentItem, int itemCount)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_ExtendedProgress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
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

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
