/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Rendering.h"

#include "addons/binary-addons/AddonDll.h"
#include "addons/interfaces/GUI/General.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/controls/Rendering.h"
#include "guilib/GUIRenderingControl.h"
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIControlAddonRendering::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_rendering = static_cast<AddonToKodiFuncTable_kodi_gui_control_rendering*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_control_rendering)));

  addonInterface->toKodi->kodi_gui->control_rendering->set_callbacks = set_callbacks;
  addonInterface->toKodi->kodi_gui->control_rendering->destroy = destroy;
}

void Interface_GUIControlAddonRendering::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->control_rendering);
}

void Interface_GUIControlAddonRendering::set_callbacks(
                            void* kodiBase, void* handle, void* clienthandle,
                            bool (*createCB)(void*,int,int,int,int,void*),
                            void (*renderCB)(void*), void (*stopCB)(void*), bool (*dirtyCB)(void*))
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonRenderingControl* control = static_cast<CGUIAddonRenderingControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlAddonRendering::%s - invalid handler data (kodiBase='%p', "
              "handle='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
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

void Interface_GUIControlAddonRendering::destroy(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonRenderingControl* control = static_cast<CGUIAddonRenderingControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlAddonRendering::%s - invalid handler data (kodiBase='%p', "
              "handle='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  static_cast<CGUIAddonRenderingControl*>(handle)->Delete();
  Interface_GUIGeneral::unlock();
}


CGUIAddonRenderingControl::CGUIAddonRenderingControl(CGUIRenderingControl *control)
  : CBCreate{nullptr},
    CBRender{nullptr},
    CBStop{nullptr},
    CBDirty{nullptr},
    m_clientHandle{nullptr},
    m_addon{nullptr},
    m_control{control},
    m_refCount{1}
{
}

bool CGUIAddonRenderingControl::Create(int x, int y, int w, int h, void *device)
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
} /* extern "C" */
