/*
 *      Copyright (C) 2005-2017 Team Kodi
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

#pragma once

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
   * Related add-on header is "./xbmc/addons/kodi-addon-dev-kit/include/kodi/gui/controls/Rendering.h"
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
    static void set_callbacks(void* kodiBase,
                             void* handle,
                             void* clienthandle,
                             bool (*createCB)(void*,int,int,int,int,void*),
                             void (*renderCB)(void*),
                             void (*stopCB)(void*),
                             bool (*dirtyCB)(void*));
    static void destroy(void* kodiBase, void* handle);
    //@}
  };

  class CGUIAddonRenderingControl : public IRenderingCallback
  {
  friend struct Interface_GUIControlAddonRendering;
  public:
    explicit CGUIAddonRenderingControl(CGUIRenderingControl *pControl);
    virtual ~CGUIAddonRenderingControl() = default;

    bool Create(int x, int y, int w, int h, void *device) override;
    void Render() override;
    void Stop() override;
    bool IsDirty() override;
    virtual void Delete();

  protected:
    bool (*CBCreate)
        (void*   cbhdl,
         int         x,
         int         y,
         int         w,
         int         h,
         void       *device);
    void (*CBRender)
        (void*   cbhdl);
    void (*CBStop)
        (void*   cbhdl);
    bool (*CBDirty)
        (void*   cbhdl);

    void* m_clientHandle;
    CAddonDll* m_addon;
    CGUIRenderingControl* m_control;
    int m_refCount;
  };

} /* namespace ADDON */
} /* extern "C" */
