/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Rendering.h"

#include "addons/binary-addons/AddonDll.h"
#include "addons/interfaces/gui/General.h"
#include "addons/kodi-dev-kit/include/kodi/gui/controls/Rendering.h"
#include "guilib/GUIRenderingControl.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIControlAddonRendering::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_rendering =
      new AddonToKodiFuncTable_kodi_gui_control_rendering();

  addonInterface->toKodi->kodi_gui->control_rendering->set_callbacks = set_callbacks;
  addonInterface->toKodi->kodi_gui->control_rendering->destroy = destroy;
}

void Interface_GUIControlAddonRendering::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->control_rendering;
}

void Interface_GUIControlAddonRendering::set_callbacks(
    KODI_HANDLE kodiBase,
    KODI_GUI_CONTROL_HANDLE handle,
    KODI_GUI_CLIENT_HANDLE clienthandle,
    bool (*createCB)(KODI_GUI_CLIENT_HANDLE, int, int, int, int, ADDON_HARDWARE_CONTEXT),
    void (*renderCB)(KODI_GUI_CLIENT_HANDLE),
    void (*stopCB)(KODI_GUI_CLIENT_HANDLE),
    bool (*dirtyCB)(KODI_GUI_CLIENT_HANDLE))
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonRenderingControl* control = static_cast<CGUIAddonRenderingControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlAddonRendering::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  control->m_clientHandle = clienthandle;
  control->CBCreate = createCB;
  control->CBRender = renderCB;
  control->CBStop = stopCB;
  control->CBDirty = dirtyCB;
  control->m_addon = addon;
  Interface_GUIGeneral::unlock();

  control->m_control->InitCallback(control);
}

void Interface_GUIControlAddonRendering::destroy(KODI_HANDLE kodiBase,
                                                 KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonRenderingControl* control = static_cast<CGUIAddonRenderingControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlAddonRendering::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  static_cast<CGUIAddonRenderingControl*>(handle)->Delete();
  Interface_GUIGeneral::unlock();
}

CGUIAddonRenderingControl::CGUIAddonRenderingControl(CGUIRenderingControl* control)
  : m_control{control}
{
}

bool CGUIAddonRenderingControl::Create(int x, int y, int w, int h, void* device)
{
  if (CBCreate)
  {
    if (CBCreate(m_clientHandle, x, y, w, h, device))
    {
      ++m_refCount;
      return true;
    }
  }
  return false;
}

void CGUIAddonRenderingControl::Render()
{
  if (CBRender)
  {
    CBRender(m_clientHandle);
  }
}

void CGUIAddonRenderingControl::Stop()
{
  if (CBStop)
  {
    CBStop(m_clientHandle);
  }

  --m_refCount;
  if (m_refCount <= 0)
    delete this;
}

void CGUIAddonRenderingControl::Delete()
{
  --m_refCount;
  if (m_refCount <= 0)
    delete this;
}

bool CGUIAddonRenderingControl::IsDirty()
{
  bool ret = true;
  if (CBDirty)
  {
    ret = CBDirty(m_clientHandle);
  }
  return ret;
}

} /* namespace ADDON */
