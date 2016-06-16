#pragma once

/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>

#include "EGLNativeType.h"
class CEGLNativeTypeAmlogic : public CEGLNativeType
{
public:
  CEGLNativeTypeAmlogic();
  virtual ~CEGLNativeTypeAmlogic();
  virtual std::string GetNativeName() const { return "amlogic"; };
  virtual bool  CheckCompatibility();
  virtual void  Initialize();
  virtual void  Destroy();
  virtual int   GetQuirks() { return EGL_QUIRK_NONE; };

  virtual bool  CreateNativeDisplay();
  virtual bool  CreateNativeWindow();
  virtual bool  GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const;
  virtual bool  GetNativeWindow(XBNativeWindowType **nativeWindow) const;

  virtual bool  DestroyNativeWindow();
  virtual bool  DestroyNativeDisplay();

  virtual bool  GetNativeResolution(RESOLUTION_INFO *res) const;
  virtual bool  SetNativeResolution(const RESOLUTION_INFO &res);
  virtual bool  ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions);
  virtual bool  GetPreferredResolution(RESOLUTION_INFO *res) const;

  virtual bool  ShowWindow(bool show);

protected:
  bool SetDisplayResolution(const char *resolution);
  void SetupVideoScaling(const char *mode);
  void DisableFreeScale();

private:
  void SetFramebufferResolution(const RESOLUTION_INFO &res) const;
  void SetFramebufferResolution(int width, int height) const;

  std::string m_framebuffer_name;
};
