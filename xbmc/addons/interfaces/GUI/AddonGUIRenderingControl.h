/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-addon-dev-kit/include/kodi/libKODI_guilib.h"
#include "guilib/IRenderingCallback.h"

class CGUIRenderingControl;

namespace KodiAPI
{
namespace GUI
{

class CGUIAddonRenderingControl : public IRenderingCallback
{
friend class CAddonCallbacksGUI;
public:
  explicit CGUIAddonRenderingControl(CGUIRenderingControl *pControl);
  ~CGUIAddonRenderingControl() override = default;
  bool Create(int x, int y, int w, int h, void *device) override;
  void Render() override;
  void Stop() override;
  bool IsDirty() override;
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
} /* namespace KodiAPI */
