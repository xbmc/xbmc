#pragma once
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

#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_LibFunc_Base.hpp"
#include "guilib/IRenderingCallback.h"

class CGUIRenderingControl;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

  class CAddOnControl_Rendering
  {
  public:
    static void Init(::V2::KodiAPI::CB_AddOnLib *callbacks);

    static void SetCallbacks(void *addonData,
                             GUIHANDLE handle,
                             GUIHANDLE clienthandle,
                             bool (*createCB)(GUIHANDLE,int,int,int,int,void*),
                             void (*renderCB)(GUIHANDLE),
                             void (*stopCB)(GUIHANDLE),
                             bool (*dirtyCB)(GUIHANDLE));
    static void Delete(void *addonData, GUIHANDLE handle);
  };

  class CGUIAddonRenderingControl : public IRenderingCallback
  {
  friend class CAddOnControl_Rendering;
  public:
    CGUIAddonRenderingControl(CGUIRenderingControl *pControl);
    virtual ~CGUIAddonRenderingControl() {}

    virtual bool Create(int x, int y, int w, int h, void *device);
    virtual void Render();
    virtual void Stop();
    virtual bool IsDirty();
    virtual void Delete();

  protected:
    bool (*CBCreate)
        (GUIHANDLE   cbhdl,
         int         x,
         int         y,
         int         w,
         int         h,
         void       *device);
    void (*CBRender)
        (GUIHANDLE   cbhdl);
    void (*CBStop)
        (GUIHANDLE   cbhdl);
    bool (*CBDirty)
        (GUIHANDLE   cbhdl);

    GUIHANDLE             m_clientHandle;
    CGUIRenderingControl *m_pControl;
    int                   m_refCount;
  };

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
