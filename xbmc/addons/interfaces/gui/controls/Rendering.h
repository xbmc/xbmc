/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/gui/controls/rendering.h"
#include "guilib/IRenderingCallback.h"

class CGUIRenderingControl;

extern "C"
{

  struct AddonGlobalInterface;

  namespace ADDON
  {

  class CAddonDll;

  /*!
   * @brief Global gui Add-on to Kodi callback functions
   *
   * To hold general gui functions and initialize also all other gui related types not
   * related to a instance type and usable for every add-on type.
   *
   * Related add-on header is "./xbmc/addons/kodi-dev-kit/include/kodi/gui/controls/Rendering.h"
   */
  struct Interface_GUIControlAddonRendering
  {
    static void Init(AddonGlobalInterface* addonInterface);
    static void DeInit(AddonGlobalInterface* addonInterface);

    /*!
     * @brief callback functions from add-on to kodi
     *
     * @note To add a new function use the "_" style to directly identify an
     * add-on callback function. Everything with CamelCase is only to be used
     * in Kodi.
     *
     * The parameter `kodiBase` is used to become the pointer for a `CAddonDll`
     * class.
     */
    //@{
    static void set_callbacks(
        KODI_HANDLE kodiBase,
        KODI_GUI_CONTROL_HANDLE handle,
        KODI_GUI_CLIENT_HANDLE clienthandle,
        bool (*createCB)(KODI_GUI_CLIENT_HANDLE, int, int, int, int, ADDON_HARDWARE_CONTEXT),
        void (*renderCB)(KODI_GUI_CLIENT_HANDLE),
        void (*stopCB)(KODI_GUI_CLIENT_HANDLE),
        bool (*dirtyCB)(KODI_GUI_CLIENT_HANDLE));
    static void destroy(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    //@}
  };

  class CGUIAddonRenderingControl : public IRenderingCallback
  {
    friend struct Interface_GUIControlAddonRendering;

  public:
    explicit CGUIAddonRenderingControl(CGUIRenderingControl* pControl);
    ~CGUIAddonRenderingControl() override = default;

    bool Create(int x, int y, int w, int h, void* device) override;
    void Render() override;
    void Stop() override;
    bool IsDirty() override;
    virtual void Delete();

  protected:
    bool (*CBCreate)(KODI_GUI_CLIENT_HANDLE cbhdl, int x, int y, int w, int h, void* device) =
        nullptr;
    void (*CBRender)(KODI_GUI_CLIENT_HANDLE cbhdl) = nullptr;
    void (*CBStop)(KODI_GUI_CLIENT_HANDLE cbhdl) = nullptr;
    bool (*CBDirty)(KODI_GUI_CLIENT_HANDLE cbhdl) = nullptr;

    KODI_GUI_CLIENT_HANDLE m_clientHandle = nullptr;
    CAddonDll* m_addon = nullptr;
    CGUIRenderingControl* m_control;
    int m_refCount = 1;
  };

  } /* namespace ADDON */
} /* extern "C" */
