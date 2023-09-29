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

/**
 * @brief A wrapper for gbm c classes to allow OOP and RAII.
 *
 */
class CGBMUtils
{
public:
  CGBMUtils(const CGBMUtils&) = delete;
  CGBMUtils& operator=(const CGBMUtils&) = delete;
  CGBMUtils() = default;
  ~CGBMUtils() = default;

  /**
   * @brief Create a gbm device for allocating buffers
   *
   * @param fd The file descriptor for a backend device
   * @return true The device creation succeeded
   * @return false The device creation failed
   */
  bool CreateDevice(int fd);

  /**
   * @brief A wrapper for gbm_device to allow OOP and RAII
   *
   */
  class CGBMDevice
  {
  public:
    CGBMDevice(const CGBMDevice&) = delete;
    CGBMDevice& operator=(const CGBMDevice&) = delete;
    explicit CGBMDevice(gbm_device* device);
    ~CGBMDevice() = default;

    /**
     * @brief Create a gbm surface
     *
     * @param width The width to use for the surface
     * @param height The height to use for the surface
     * @param format The format to use for the surface
     * @param modifiers The modifiers to use for the surface
     * @param modifiers_count The amount of modifiers in the modifiers param
     * @return true The surface creation succeeded
     * @return false The surface creation failed
     */
    bool CreateSurface(int width,
                       int height,
                       uint32_t format,
                       const uint64_t* modifiers,
                       const int modifiers_count);

    /**
     * @brief Get the underlying gbm_device
     *
     * @return gbm_device* A pointer to the underlying gbm_device
     */
    gbm_device* Get() const { return m_device; }

    /**
     * @brief A wrapper for gbm_surface to allow OOP and RAII
     *
     */
    class CGBMSurface
    {
    public:
      CGBMSurface(const CGBMSurface&) = delete;
      CGBMSurface& operator=(const CGBMSurface&) = delete;
      explicit CGBMSurface(gbm_surface* surface);
      ~CGBMSurface() = default;

      /**
       * @brief Get the underlying gbm_surface
       *
       * @return gbm_surface* A pointer to the underlying gbm_surface
       */
      gbm_surface* Get() const { return m_surface; }

      /**
       * @brief A wrapper for gbm_bo to allow OOP and RAII
       *
       */
      class CGBMSurfaceBuffer
      {
      public:
        CGBMSurfaceBuffer(const CGBMSurfaceBuffer&) = delete;
        CGBMSurfaceBuffer& operator=(const CGBMSurfaceBuffer&) = delete;
        explicit CGBMSurfaceBuffer(gbm_surface* surface);
        ~CGBMSurfaceBuffer();

        /**
         * @brief Get the underlying gbm_bo
         *
         * @return gbm_bo* A pointer to the underlying gbm_bo
         */
        gbm_bo* Get() const { return m_buffer; }

      private:
        gbm_surface* m_surface{nullptr};
        gbm_bo* m_buffer{nullptr};
      };

      /**
       * @brief Lock the surface's current front buffer.
       *
       * @return CGBMSurfaceBuffer* A pointer to a CGBMSurfaceBuffer object
       */
      CGBMSurfaceBuffer& LockFrontBuffer();

    private:
      gbm_surface* m_surface{nullptr};
      std::queue<std::unique_ptr<CGBMSurfaceBuffer>> m_buffers;
    };

    /**
     * @brief Get the CGBMSurface object
     *
     * @return CGBMSurface* A pointer to the CGBMSurface object
     */
    CGBMDevice::CGBMSurface& GetSurface() const { return *m_surface; }

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

  /**
   * @brief Get the CGBMDevice object
   *
   * @return CGBMDevice* A pointer to the CGBMDevice object
   */
  CGBMUtils::CGBMDevice& GetDevice() const { return *m_device; }

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
