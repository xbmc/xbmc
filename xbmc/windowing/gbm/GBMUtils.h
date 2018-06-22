/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <gbm.h>

class CGBMUtils
{
public:
  CGBMUtils() = default;
  ~CGBMUtils() = default;
  bool CreateDevice(int fd);
  void DestroyDevice();
  bool CreateSurface(int width, int height);
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
