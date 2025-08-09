/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Progress.h"

#include "ServiceBroker.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/dialogs/Progress.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/Variant.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIDialogProgress::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogProgress =
      new AddonToKodiFuncTable_kodi_gui_dialogProgress();

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
  delete addonInterface->toKodi->kodi_gui->dialogProgress;
}

KODI_GUI_HANDLE Interface_GUIDialogProgress::new_dialog(KODI_HANDLE kodiBase)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return nullptr;
  }

  CGUIDialogProgress* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(
          WINDOW_DIALOG_PROGRESS);
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Invalid handler data (dialog='{}') on addon '{}'",
               static_cast<void*>(dialog), addon->ID());
    return nullptr;
  }

  return dialog;
}

void Interface_GUIDialogProgress::delete_dialog(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return;
  }

  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data (handle='{}') on addon '{}'", handle, addon->ID());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->Close();
}

void Interface_GUIDialogProgress::open(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return;
  }

  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data (handle='{}') on addon '{}'", handle, addon->ID());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->Open();
}

void Interface_GUIDialogProgress::set_heading(KODI_HANDLE kodiBase,
                                              KODI_GUI_HANDLE handle,
                                              const char* heading)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return;
  }

  if (!handle || !heading)
  {
    CLog::LogF(LOGERROR,
               "Invalid handler data (handle='{}', heading='{}') "
               "on addon '{}'",
               handle, static_cast<const void*>(heading), addon->ID());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->SetHeading(heading);
}

void Interface_GUIDialogProgress::set_line(KODI_HANDLE kodiBase,
                                           KODI_GUI_HANDLE handle,
                                           unsigned int line,
                                           const char* text)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return;
  }

  if (!handle || !text)
  {
    CLog::LogF(LOGERROR,
               "Invalid handler data (handle='{}', text='{}') on "
               "addon '{}'",
               handle, static_cast<const void*>(text), addon->ID());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->SetLine(line, text);
}

void Interface_GUIDialogProgress::set_can_cancel(KODI_HANDLE kodiBase,
                                                 KODI_GUI_HANDLE handle,
                                                 bool canCancel)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return;
  }

  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data (handle='{}') on addon '{}'", handle, addon->ID());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->SetCanCancel(canCancel);
}

bool Interface_GUIDialogProgress::is_canceled(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return false;
  }

  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data (handle='{}') on addon '{}'", handle, addon->ID());
    return false;
  }

  return static_cast<CGUIDialogProgress*>(handle)->IsCanceled();
}

void Interface_GUIDialogProgress::set_percentage(KODI_HANDLE kodiBase,
                                                 KODI_GUI_HANDLE handle,
                                                 int percentage)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return;
  }

  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data (handle='{}') on addon '{}'", handle, addon->ID());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->SetPercentage(percentage);
}

int Interface_GUIDialogProgress::get_percentage(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return 0;
  }

  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data (handle='{}') on addon '{}'", handle, addon->ID());
    return 0;
  }

  return static_cast<CGUIDialogProgress*>(handle)->GetPercentage();
}

void Interface_GUIDialogProgress::show_progress_bar(KODI_HANDLE kodiBase,
                                                    KODI_GUI_HANDLE handle,
                                                    bool onOff)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return;
  }

  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data (handle='{}') on addon '{}'", handle, addon->ID());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->ShowProgressBar(onOff);
}

void Interface_GUIDialogProgress::set_progress_max(KODI_HANDLE kodiBase,
                                                   KODI_GUI_HANDLE handle,
                                                   int max)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return;
  }

  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data (handle='{}') on addon '{}'", handle, addon->ID());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->SetProgressMax(max);
}

void Interface_GUIDialogProgress::set_progress_advance(KODI_HANDLE kodiBase,
                                                       KODI_GUI_HANDLE handle,
                                                       int nSteps)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return;
  }

  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data (handle='{}') on addon '{}'", handle, addon->ID());
    return;
  }

  static_cast<CGUIDialogProgress*>(handle)->SetProgressAdvance(nSteps);
}

bool Interface_GUIDialogProgress::abort(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return false;
  }

  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data (handle='{}') on addon '{}'", handle, addon->ID());
    return false;
  }

  return static_cast<CGUIDialogProgress*>(handle)->Abort();
}

} /* namespace ADDON */
