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

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/AddonInterfaces.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
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

void CAddOnDialog_Progress::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Dialogs.Progress.New                = CAddOnDialog_Progress::New;
  interfaces->GUI.Dialogs.Progress.Delete             = CAddOnDialog_Progress::Delete;
  interfaces->GUI.Dialogs.Progress.Open               = CAddOnDialog_Progress::Open;
  interfaces->GUI.Dialogs.Progress.SetHeading         = CAddOnDialog_Progress::SetHeading;
  interfaces->GUI.Dialogs.Progress.SetLine            = CAddOnDialog_Progress::SetLine;
  interfaces->GUI.Dialogs.Progress.SetCanCancel       = CAddOnDialog_Progress::SetCanCancel;
  interfaces->GUI.Dialogs.Progress.IsCanceled         = CAddOnDialog_Progress::IsCanceled;
  interfaces->GUI.Dialogs.Progress.SetPercentage      = CAddOnDialog_Progress::SetPercentage;
  interfaces->GUI.Dialogs.Progress.GetPercentage      = CAddOnDialog_Progress::GetPercentage;
  interfaces->GUI.Dialogs.Progress.ShowProgressBar    = CAddOnDialog_Progress::ShowProgressBar;
  interfaces->GUI.Dialogs.Progress.SetProgressMax     = CAddOnDialog_Progress::SetProgressMax;
  interfaces->GUI.Dialogs.Progress.SetProgressAdvance = CAddOnDialog_Progress::SetProgressAdvance;
  interfaces->GUI.Dialogs.Progress.Abort              = CAddOnDialog_Progress::Abort;
}

void* CAddOnDialog_Progress::New(void *addonData)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    CGUIDialogProgress *dialog = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    return dialog;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnDialog_Progress::Delete(void *addonData, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_Progress::Open(void *addonData, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_Progress::SetHeading(void *addonData, void* handle, const char *heading)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle || !heading)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_Progress::SetLine(void *addonData, void* handle, unsigned int iLine, const char *line)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle || !line)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_Progress::SetCanCancel(void *addonData, void* handle, bool bCanCancel)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
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

bool CAddOnDialog_Progress::IsCanceled(void *addonData, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_Progress::SetPercentage(void *addonData, void* handle, int iPercentage)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
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

int CAddOnDialog_Progress::GetPercentage(void *addonData, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_Progress::ShowProgressBar(void *addonData, void* handle, bool bOnOff)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_Progress::SetProgressMax(void *addonData, void* handle, int iMax)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
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

void CAddOnDialog_Progress::SetProgressAdvance(void *addonData, void* handle, int nSteps)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
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

bool CAddOnDialog_Progress::Abort(void *addonData, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnDialog_Progress - %s - invalid add-on data", __FUNCTION__);

    if (!handle)
    {
      CAddonInterfaceAddon* guiHelper = static_cast<V2::KodiAPI::CAddonInterfaceAddon*>(helper->AddOnLib_GetHelper());
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

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
