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

#include "Progress.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/dialogs/Progress.h"

#include "addons/binary-addons/AddonDll.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "ServiceBroker.h"

namespace ADDON
{
extern "C"
{

void Interface_GUIDialogProgress::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogProgress = static_cast<AddonToKodiFuncTable_kodi_gui_dialogProgress*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_dialogProgress)));

  addonInterface->toKodi->kodi_gui->dialogProgress->new_dialog = new_dialog;
  addonInterface->toKodi->kodi_gui->dialogProgress->delete_dialog = delete_dialog;
  addonInterface->toKodi->kodi_gui->dialogProgress->open = open;
  addonInterface->toKodi->kodi_gui->dialogProgress->set_heading = set_heading;
  addonInterface->toKodi->kodi_gui->dialogProgress->set_line = set_line;
  addonInterface->toKodi->kodi_gui->dialogProgress->set_can_cancel = set_can_cancel;
  addonInterface->toKodi->kodi_gui->dialogProgress->is_canceled = is_canceled;
  addonInterface->toKodi->kodi_gui->dialogProgress->set_percentage = set_percentage;
  addonInterface->toKodi->kodi_gui->dialogProgress->get_percentage = get_percentage;
  addonInterface->toKodi->kodi_gui->dialogProgress->show_progress_bar = show_progress_bar;
  addonInterface->toKodi->kodi_gui->dialogProgress->set_progress_max = set_progress_max;
  addonInterface->toKodi->kodi_gui->dialogProgress->set_progress_advance = set_progress_advance;
  addonInterface->toKodi->kodi_gui->dialogProgress->abort = abort;
}

void Interface_GUIDialogProgress::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->dialogProgress);
}

void* Interface_GUIDialogProgress::new_dialog(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return nullptr;
  }

  CGUIDialogProgress *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
  if (!dialog)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (dialog='%p') on addon '%s'",
              __FUNCTION__, static_cast<void*>(dialog), addon->ID().c_str());
    return nullptr;
  }

  return dialog;
}

void Interface_GUIDialogProgress::delete_dialog(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (handle='%p') on addon '%s'",
              __FUNCTION__, handle, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->Close();
}

void Interface_GUIDialogProgress::open(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (handle='%p') on addon '%s'",
              __FUNCTION__, handle, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->Open();
}

void Interface_GUIDialogProgress::set_heading(void* kodiBase, void* handle, const char* heading)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return;
  }

  if (!handle || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (handle='%p', heading='%p') "
              "on addon '%s'",
              __FUNCTION__, handle, static_cast<const void*>(heading), addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->SetHeading(heading);
}

void Interface_GUIDialogProgress::set_line(void* kodiBase, void* handle, unsigned int line, const char* text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return;
  }

  if (!handle || !text)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (handle='%p', text='%p') on "
              "addon '%s'",
              __FUNCTION__, handle, static_cast<const void*>(text), addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->SetLine(line, text);
}

void Interface_GUIDialogProgress::set_can_cancel(void* kodiBase, void* handle, bool canCancel)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (handle='%p') on addon '%s'",
              __FUNCTION__, handle, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->SetCanCancel(canCancel);
}

bool Interface_GUIDialogProgress::is_canceled(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (handle='%p') on addon '%s'",
              __FUNCTION__, handle, addon->ID().c_str());
    return false;
  }

  return static_cast<CGUIDialogProgress*>(handle)->IsCanceled();
}

void Interface_GUIDialogProgress::set_percentage(void* kodiBase, void* handle, int percentage)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (handle='%p') on addon '%s'",
              __FUNCTION__, handle, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->SetPercentage(percentage);
}

int Interface_GUIDialogProgress::get_percentage(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return 0;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (handle='%p') on addon '%s'",
              __FUNCTION__, handle, addon->ID().c_str());
    return 0;
  }

  return static_cast<CGUIDialogProgress*>(handle)->GetPercentage();
}

void Interface_GUIDialogProgress::show_progress_bar(void* kodiBase, void* handle, bool onOff)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (handle='%p') on addon '%s'",
              __FUNCTION__, handle, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->ShowProgressBar(onOff);
}

void Interface_GUIDialogProgress::set_progress_max(void* kodiBase, void* handle, int max)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (handle='%p') on addon '%s'",
              __FUNCTION__, handle, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->SetProgressMax(max);
}

void Interface_GUIDialogProgress::set_progress_advance(void* kodiBase, void* handle, int nSteps)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (handle='%p') on addon '%s'",
              __FUNCTION__, handle, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->SetProgressAdvance(nSteps);
}

bool Interface_GUIDialogProgress::abort(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogProgress::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogProgress::%s - invalid handler data (handle='%p') on addon '%s'",
              __FUNCTION__, handle, addon->ID().c_str());
    return false;
  }

  return static_cast<CGUIDialogProgress*>(handle)->Abort();
}

} /* extern "C" */
} /* namespace ADDON */
