/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <queue>

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
  CGBMUtils(const CGBMUtils&) = delete;
  CGBMUtils& operator=(const CGBMUtils&) = delete;
  CGBMUtils() = default;
  ~CGBMUtils() = default;
  bool CreateDevice(int fd);

  class CGBMDevice
  {
  public:
    CGBMDevice(const CGBMDevice&) = delete;
    CGBMDevice& operator=(const CGBMDevice&) = delete;
    explicit CGBMDevice(gbm_device* device);
    ~CGBMDevice() = default;

    bool CreateSurface(int width,
                       int height,
                       uint32_t format,
                       const uint64_t* modifiers,
                       const int modifiers_count);

    gbm_device* Get() const { return m_device; }

    class CGBMSurface
    {
    public:
      CGBMSurface(const CGBMSurface&) = delete;
      CGBMSurface& operator=(const CGBMSurface&) = delete;
      explicit CGBMSurface(gbm_surface* surface);
      ~CGBMSurface() = default;

      gbm_surface* Get() const { return m_surface; }

      class CGBMSurfaceBuffer
      {
      public:
        CGBMSurfaceBuffer(const CGBMSurfaceBuffer&) = delete;
        CGBMSurfaceBuffer& operator=(const CGBMSurfaceBuffer&) = delete;
        explicit CGBMSurfaceBuffer(gbm_surface* surface);
        ~CGBMSurfaceBuffer();

        gbm_bo* Get() const { return m_buffer; }

      private:
        gbm_surface* m_surface{nullptr};
        gbm_bo* m_buffer{nullptr};
      };

      CGBMSurfaceBuffer* LockFrontBuffer();

    private:
      gbm_surface* m_surface{nullptr};
      std::queue<std::unique_ptr<CGBMSurfaceBuffer>> m_buffers;
    };

    CGBMDevice::CGBMSurface* GetSurface() const { return m_surface.get(); }

  private:
    gbm_device* m_device{nullptr};

    struct CGBMSurfaceDeleter
    {
      void operator()(CGBMSurface* p) const
      {
        if (p)
          gbm_surface_destroy(p->Get());
      }
    };
    std::unique_ptr<CGBMSurface, CGBMSurfaceDeleter> m_surface;
  };

  CGBMUtils::CGBMDevice* GetDevice() const { return m_device.get(); }

private:
  struct CGBMDeviceDeleter
  {
    void operator()(CGBMDevice* p) const
    {
      if (p)
        gbm_device_destroy(p->Get());
    }
  };
  std::unique_ptr<CGBMDevice, CGBMDeviceDeleter> m_device;
};

}
}
}
