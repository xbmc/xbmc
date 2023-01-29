/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>

#include <X11/Xlib.h>

class CGLContext
{
public:
  explicit CGLContext(Display *dpy)
  {
    m_dpy = dpy;
  }
  virtual ~CGLContext() = default;
  virtual bool Refresh(bool force, int screen, Window glWindow, bool &newContext) = 0;
  virtual bool CreatePB() { return false; }
  virtual void Destroy() = 0;
  virtual void Detach() = 0;
  virtual void SetVSync(bool enable) = 0;
  virtual void SwapBuffers() = 0;
  virtual void QueryExtensions() = 0;
  virtual uint64_t GetVblankTiming(uint64_t& msc, uint64_t& interval) { return 0; }
  bool IsExtSupported(const char* extension) const;

  std::string ExtPrefix() { return m_extPrefix; }
  std::string m_extPrefix;
  std::string m_extensions;

  Display *m_dpy;

protected:
  bool m_omlSync = true;
};
