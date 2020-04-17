/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>

/**
 * @brief Interface to describe CBufferObjects.
 *
 *        BufferObjects are used to abstract various memory types and present them
 *        with a generic interface. Typically on a posix system exists the concept
 *        of Direct Memory Access (DMA) which allows various systems to interact
 *        with a memory location directly without having to copy the data into
 *        userspace (ie. from kernel space). These DMA buffers are presented as
 *        file descriptors (fds) which can be passed around to gain access to
 *        the buffers memory location.
 *
 *        In order to write to these buffer types typically the memory location has
 *        to be mapped into userspace via a call to mmap (or similar). This presents
 *        userspace with a memory location that can be initially written to (ie. by
 *        using memcpy or similar). Depending on the underlying implementation a
 *        stride might be specified if the memory type requires padding to a certain
 *        size (such as page size). The stride must be used when copying into the buffer.
 *
 *        Some memory implementation may provide special memory layouts in which case
 *        modifiers are provided that describe tiling or compression. This should be
 *        transparent to the caller as the data copied into a BufferObject will likely
 *        be linear. The modifier will be needed when presenting the buffer via DRM or
 *        EGL even if it is linear.
 *
 */
class IBufferObject
{
public:
  virtual ~IBufferObject() = default;

  /**
   * @brief Create a BufferObject based on the format, width, and height of the desired buffer
   *
   * @param format framebuffer pixel formats are described using the fourcc codes defined in
   *               https://github.com/torvalds/linux/blob/master/include/uapi/drm/drm_fourcc.h
   * @param width width of the requested buffer.
   * @param height height of the requested buffer.
   * @return true BufferObject creation was successful.
   * @return false BufferObject creation was unsuccessful.
   */
  virtual bool CreateBufferObject(uint32_t format, uint32_t width, uint32_t height) = 0;

  /**
   * @brief Create a BufferObject based only on the size of the desired buffer. Not all
   *        CBufferObject implementations may support this. This method is required for
   *        use with the CAddonVideoCodec as it only knows the decoded buffer size.
   *
   * @param size of the requested buffer.
   * @return true BufferObject creation was successful.
   * @return false BufferObject creation was unsuccessful.
   */
  virtual bool CreateBufferObject(uint64_t size) = 0;

  /**
   * @brief Destroy a BufferObject.
   *
   */
  virtual void DestroyBufferObject() = 0;

  /**
   * @brief Get the Memory location of the BufferObject. This method needs to be
   *        called to be able to copy data into the BufferObject.
   *
   * @return uint8_t* pointer to the memory location of the BufferObject.
   */
  virtual uint8_t *GetMemory() = 0;

  /**
   * @brief Release the mapped memory of the BufferObject. After calling this the memory
   *        location pointed to by GetMemory() will be invalid.
   *
   */
  virtual void ReleaseMemory() = 0;

  /**
   * @brief Get the File Descriptor of the BufferObject. The fd is guaranteed to be
   *        available after calling CreateBufferObject().
   *
   * @return int fd for the BufferObject. Invalid if -1.
   */
  virtual int GetFd() = 0;

  /**
   * @brief Get the Stride of the BufferObject. The stride is guaranteed to be
   *        available after calling GetMemory().
   *
   * @return uint32_t stride of the BufferObject.
   */
  virtual uint32_t GetStride() = 0;

  /**
   * @brief Get the Modifier of the BufferObject. Format Modifiers further describe
   *        the buffer's format such as for tiling or compression.
   *        see https://github.com/torvalds/linux/blob/master/include/uapi/drm/drm_fourcc.h
   *
   * @return uint64_t modifier of the BufferObject. 0 means the layout is linear (default).
   */
  virtual uint64_t GetModifier() = 0;

  /**
   * @brief Must be called before reading/writing data to the BufferObject.
   *
   */
  virtual void SyncStart() = 0;

  /**
   * @brief Must be called after reading/writing data to the BufferObject.
   *
   */
  virtual void SyncEnd() = 0;

  /**
   * @brief Get the Name of the BufferObject type in use
   *
   * @return std::string name of the BufferObject type in use
   */
  virtual std::string GetName() const = 0;
};
