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

#include "addons/kodi-addon-dev-kit/include/kodi/libKODI_guilib.h"
#include "guilib/IRenderingCallback.h"

class CGUIRenderingControl;

namespace KodiAPI
{
namespace V1
{
namespace GUI
{

class CGUIAddonRenderingControl : public IRenderingCallback
{
friend class CAddonCallbacksGUI;
public:
  CGUIAddonRenderingControl(CGUIRenderingControl *pControl);
  virtual ~CGUIAddonRenderingControl() {}
  virtual bool Create(int x, int y, int w, int h, void *device);
  virtual void Render();
  virtual void Stop();
  virtual bool IsDirty();
  virtual void Delete();
protected:
  bool (*CBCreate) (GUIHANDLE cbhdl, int x, int y, int w, int h, void *device);
  void (*CBRender)(GUIHANDLE cbhdl);
  void (*CBStop)(GUIHANDLE cbhdl);
  bool (*CBDirty)(GUIHANDLE cbhdl);

  GUIHANDLE m_clientHandle;
  CGUIRenderingControl *m_pControl;
  int m_refCount;
};

} /* namespace GUI */
} /* namespace V1 */
} /* namespace KodiAPI */
