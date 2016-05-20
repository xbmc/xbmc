#pragma once
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

#include "guilib/IRenderingCallback.h"

class CGUIRenderingControl;

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;

namespace GUI
{
extern "C"
{

  class CAddOnControl_Rendering
  {
  public:
    static void Init(struct CB_AddOnLib *interfaces);

    static void SetCallbacks(void *addonData,
                             void* handle,
                             void* clienthandle,
                             bool (*createCB)(void*,int,int,int,int,void*),
                             void (*renderCB)(void*),
                             void (*stopCB)(void*),
                             bool (*dirtyCB)(void*));
    static void Delete(void *addonData, void* handle);
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

    void*                 m_clientHandle;
    CGUIRenderingControl *m_pControl;
    int                   m_refCount;
  };

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
