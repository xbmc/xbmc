/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system_egl.h"

#include <EGL/eglext.h>

namespace KODI
{
namespace UTILS
{
namespace EGL
{

class CEGLFence
{
public:
  explicit CEGLFence(EGLDisplay display);
  CEGLFence(CEGLFence const& other) = delete;
  CEGLFence& operator=(CEGLFence const& other) = delete;

  void CreateFence();
  void DestroyFence();
  bool IsSignaled();

private:
  EGLDisplay m_display{nullptr};
  EGLSyncKHR m_fence{nullptr};

  PFNEGLCREATESYNCKHRPROC m_eglCreateSyncKHR{nullptr};
  PFNEGLDESTROYSYNCKHRPROC m_eglDestroySyncKHR{nullptr};
  PFNEGLGETSYNCATTRIBKHRPROC m_eglGetSyncAttribKHR{nullptr};
};

}
}
}
