/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ExtendedProgressBar.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/dialogs/ExtendedProgress.h"

#include "addons/binary-addons/AddonDll.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "ServiceBroker.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIDialogExtendedProgress::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogExtendedProgress = static_cast<AddonToKodiFuncTable_kodi_gui_dialogExtendedProgress*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_dialogExtendedProgress)));

  addonInterface->toKodi->kodi_gui->dialogExtendedProgress->new_dialog  = new_dialog;
  addonInterface->toKodi->kodi_gui->dialogExtendedProgress->delete_dialog = delete_dialog;
  addonInterface->toKodi->kodi_gui->dialogExtendedProgress->get_title = get_title;
  addonInterface->toKodi->kodi_gui->dialogExtendedProgress->set_title = set_title;
  addonInterface->toKodi->kodi_gui->dialogExtendedProgress->get_text = get_text;
  addonInterface->toKodi->kodi_gui->dialogExtendedProgress->set_text = set_text;
  addonInterface->toKodi->kodi_gui->dialogExtendedProgress->is_finished = is_finished;
  addonInterface->toKodi->kodi_gui->dialogExtendedProgress->mark_finished = mark_finished;
  addonInterface->toKodi->kodi_gui->dialogExtendedProgress->get_percentage = get_percentage;
  addonInterface->toKodi->kodi_gui->dialogExtendedProgress->set_percentage = set_percentage;
  addonInterface->toKodi->kodi_gui->dialogExtendedProgress->set_progress = set_progress;
}

void Interface_GUIDialogExtendedProgress::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->dialogExtendedProgress);
}

void* Interface_GUIDialogExtendedProgress::new_dialog(void* kodiBase, const char *title)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogExtendedProgress::%s - invalid kodi base data", __FUNCTION__);
    return nullptr;
  }

  // setup the progress dialog
  CGUIDialogExtendedProgressBar* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(WINDOW_DIALOG_EXT_PROGRESS);
  if (!title || !dialog)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogExtendedProgress::%s - invalid handler data (title='%p', "
              "dialog='%p') on addon '%s'",
              __FUNCTION__, title, static_cast<void*>(dialog), addon->ID().c_str());
    return nullptr;
  }

  CGUIDialogProgressBarHandle* dlgProgressHandle = dialog->GetHandle(title);
  return dlgProgressHandle;
}

void Interface_GUIDialogExtendedProgress::delete_dialog(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogExtendedProgress::%s - invalid kodi base data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogExtendedProgress::%s - invalid handler data (handle='%p') on addon '%s'", __FUNCTION__, handle, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgressBarHandle*>(handle)->MarkFinished();
}

char* Interface_GUIDialogExtendedProgress::get_title(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogExtendedProgress::%s - invalid kodi base data", __FUNCTION__);
    return nullptr;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogExtendedProgress::%s - invalid handler data (handle='%p') on "
              "addon '%s'",
              __FUNCTION__, handle, addon->ID().c_str());
    return nullptr;
  }

  return strdup(static_cast<CGUIDialogProgressBarHandle*>(handle)->Title().c_str());
}

void Interface_GUIDialogExtendedProgress::set_title(void* kodiBase, void* handle, const char *title)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogExtendedProgress::%s - invalid kodi base data", __FUNCTION__);
    return;
  }

  if (!handle || !title)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogExtendedProgress::%s - invalid handler data (handle='%p', "
              "title='%p') on addon '%s'",
              __FUNCTION__, handle, title, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgressBarHandle*>(handle)->SetTitle(title);
}

char* Interface_GUIDialogExtendedProgress::get_text(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogExtendedProgress::%s - invalid kodi base data", __FUNCTION__);
    return nullptr;
  }

  if (!handle)
  {
    CLog::Log(
        LOGERROR,
        "Interface_GUIDialogExtendedProgress::%s - invalid add-on data (handle='%p') on addon '%s'",
        __FUNCTION__, handle, addon->ID().c_str());
    return nullptr;
  }

  return strdup(static_cast<CGUIDialogProgressBarHandle*>(handle)->Text().c_str());
}

void Interface_GUIDialogExtendedProgress::set_text(void* kodiBase, void* handle, const char *text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogExtendedProgress::%s - invalid kodi base data", __FUNCTION__);
    return;
  }

  if (!handle || !text)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogExtendedProgress::%s - invalid handler data (handle='%p', "
              "text='%p') on addon '%s'",
              __FUNCTION__, handle, text, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgressBarHandle*>(handle)->SetText(text);
}

bool Interface_GUIDialogExtendedProgress::is_finished(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogExtendedProgress::%s - invalid kodi base data", __FUNCTION__);
    return false;
  }

  if (!handle)
  {
    CLog::Log(
        LOGERROR,
        "Interface_GUIDialogExtendedProgress::%s - invalid add-on data (handle='%p') on addon '%s'",
        __FUNCTION__, handle, addon->ID().c_str());
    return false;
  }

  return static_cast<CGUIDialogProgressBarHandle*>(handle)->IsFinished();
}

void Interface_GUIDialogExtendedProgress::mark_finished(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogExtendedProgress::%s - invalid kodi base data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(
        LOGERROR,
        "Interface_GUIDialogExtendedProgress::%s - invalid add-on data (handle='%p') on addon '%s'",
        __FUNCTION__, handle, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgressBarHandle*>(handle)->MarkFinished();
}

float Interface_GUIDialogExtendedProgress::get_percentage(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogExtendedProgress::%s - invalid kodi base data", __FUNCTION__);
    return 0.0f;
  }

  if (!handle)
  {
    CLog::Log(
        LOGERROR,
        "Interface_GUIDialogExtendedProgress::%s - invalid add-on data (handle='%p') on addon '%s'",
        __FUNCTION__, handle, addon->ID().c_str());
    return 0.0f;
  }

  return static_cast<CGUIDialogProgressBarHandle*>(handle)->Percentage();
}

void Interface_GUIDialogExtendedProgress::set_percentage(void* kodiBase, void* handle, float percentage)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogExtendedProgress::%s - invalid kodi base data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(
        LOGERROR,
        "Interface_GUIDialogExtendedProgress::%s - invalid add-on data (handle='%p') on addon '%s'",
        __FUNCTION__, handle, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgressBarHandle*>(handle)->SetPercentage(percentage);
}

void Interface_GUIDialogExtendedProgress::set_progress(void* kodiBase, void* handle, int currentItem, int itemCount)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogExtendedProgress::%s - invalid kodi base data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(
        LOGERROR,
        "Interface_GUIDialogExtendedProgress::%s - invalid add-on data (handle='%p') on addon '%s'",
        __FUNCTION__, handle, addon->ID().c_str());
    return;
  }

  static_cast<CGUIDialogProgressBarHandle*>(handle)->SetProgress(currentItem, itemCount);
}

} /* namespace ADDON */
} /* extern "C" */
