/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <gbm.h>

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class CGBMUtils
{
public:
  CGBMUtils() = default;
  ~CGBMUtils() = default;
  bool CreateDevice(int fd);
  void DestroyDevice();
  bool CreateSurface(int width, int height, uint32_t format, const uint64_t *modifiers, const int modifiers_count);
  void DestroySurface();
  struct gbm_bo *LockFrontBuffer();
  void ReleaseBuffer();

  struct gbm_device* GetDevice() const { return m_device; }
  struct gbm_surface* GetSurface() const { return m_surface; }

protected:
  struct gbm_device *m_device = nullptr;
  struct gbm_surface *m_surface = nullptr;
  struct gbm_bo *m_bo = nullptr;
  struct gbm_bo *m_next_bo = nullptr;
};

}
}
}
