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
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "utils/Variant.h"

#include "AddonGUIDialogProgress.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnDialog_Progress::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->GUI.Dialogs.Progress.New                = CAddOnDialog_Progress::New;
  callbacks->GUI.Dialogs.Progress.Delete             = CAddOnDialog_Progress::Delete;
  callbacks->GUI.Dialogs.Progress.Open               = CAddOnDialog_Progress::Open;
  callbacks->GUI.Dialogs.Progress.SetHeading         = CAddOnDialog_Progress::SetHeading;
  callbacks->GUI.Dialogs.Progress.SetLine            = CAddOnDialog_Progress::SetLine;
  callbacks->GUI.Dialogs.Progress.SetCanCancel       = CAddOnDialog_Progress::SetCanCancel;
  callbacks->GUI.Dialogs.Progress.IsCanceled         = CAddOnDialog_Progress::IsCanceled;
  callbacks->GUI.Dialogs.Progress.SetPercentage      = CAddOnDialog_Progress::SetPercentage;
  callbacks->GUI.Dialogs.Progress.GetPercentage      = CAddOnDialog_Progress::GetPercentage;
  callbacks->GUI.Dialogs.Progress.ShowProgressBar    = CAddOnDialog_Progress::ShowProgressBar;
  callbacks->GUI.Dialogs.Progress.SetProgressMax     = CAddOnDialog_Progress::SetProgressMax;
  callbacks->GUI.Dialogs.Progress.SetProgressAdvance = CAddOnDialog_Progress::SetProgressAdvance;
  callbacks->GUI.Dialogs.Progress.Abort              = CAddOnDialog_Progress::Abort;
}

GUIHANDLE CAddOnDialog_Progress::New(void *addonData)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    CGUIDialogProgress *dialog = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    return dialog;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnDialog_Progress::Delete(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress *>(handle);
    dialog->Close();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDialog_Progress::Open(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress *>(handle);
    dialog->Open();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDialog_Progress::SetHeading(void *addonData, GUIHANDLE handle, const char *heading)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle || !heading)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s: %s/%s - No Dialog with invalid handler data or nullptr heading",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress *>(handle);
    dialog->SetHeading(heading);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDialog_Progress::SetLine(void *addonData, GUIHANDLE handle, unsigned int iLine, const char *line)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle || !line)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s: %s/%s - No Dialog with invalid handler data or nullptr line",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress *>(handle);
    dialog->SetLine(iLine, line);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDialog_Progress::SetCanCancel(void *addonData, GUIHANDLE handle, bool bCanCancel)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress *>(handle);
    dialog->SetCanCancel(bCanCancel);
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnDialog_Progress::IsCanceled(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress *>(handle);
    return dialog->IsCanceled();
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnDialog_Progress::SetPercentage(void *addonData, GUIHANDLE handle, int iPercentage)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress *>(handle);
    dialog->SetPercentage(iPercentage);
  }
  HANDLE_ADDON_EXCEPTION
}

int CAddOnDialog_Progress::GetPercentage(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress *>(handle);
    return dialog->GetPercentage();
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

void CAddOnDialog_Progress::ShowProgressBar(void *addonData, GUIHANDLE handle, bool bOnOff)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress *>(handle);
    dialog->ShowProgressBar(bOnOff);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDialog_Progress::SetProgressMax(void *addonData, GUIHANDLE handle, int iMax)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress *>(handle);
    dialog->SetProgressMax(iMax);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDialog_Progress::SetProgressAdvance(void *addonData, GUIHANDLE handle, int nSteps)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress *>(handle);
    dialog->SetProgressAdvance(nSteps);
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnDialog_Progress::Abort(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s: %s/%s - No Dialog with invalid handler data",
                                         __FUNCTION__,
                                         TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                                         guiHelper->GetAddon()->Name().c_str());
    }

    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress *>(handle);
    return dialog->Abort();
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
