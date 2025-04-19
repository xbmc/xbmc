/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace KODI
{
namespace RETRO
{
class IRetroPlayerStream;
}

namespace SMART_HOME
{

class ISmartHomeStream
{
public:
  virtual ~ISmartHomeStream() = default;

  /*!
   * \brief Open the stream
   *
   * \param stream The RetroPlayer resource to take ownership of
   * \param nonimalFormat The expected format of stream images
   * \param nominalWidth The expected width of stream images
   * \param nominalHeight The expected height of stream images
   *
   * \return True if the stream was opened, false otherwise
   */
  virtual bool OpenStream(RETRO::IRetroPlayerStream* stream,
                          AVPixelFormat nominalFormat,
                          unsigned int nominalWidth,
                          unsigned int nominalHeight) = 0;

  /*!
   * \brief Release the RetroPlayer stream resource
   */
  virtual void CloseStream() = 0;

  /*!
   * \brief Get a buffer for zero-copy stream data
   *
   * \param width The framebuffer width, or 0 for no width specified
   * \param height The framebuffer height, or 0 for no height specified
   * \param[out] format The libav pixel format of the buffer, ignored if false is returned
   * \param[out] data The buffer data, ignored if false is returned
   * \param[out] size The size of the data, ignored if false is returned
   *
   * If this returns true, buffer must be freed using ReleaseBuffer().
   *
   * \return True if buffer was set, false otherwise
   */
  virtual bool GetBuffer(unsigned int width,
                         unsigned int height,
                         AVPixelFormat& format,
                         uint8_t*& data,
                         size_t& size) = 0;

  /*!
   * \brief Free an allocated buffer
   *
   * \param buffer The buffer returned from GetBuffer()
   */
  virtual void ReleaseBuffer(uint8_t* data) = 0;

  /*!
   * \brief Add a data packet to a stream
   *
   * \param width The frame width
   * \param height The frame height
   * \param data Pointer for stream data given to Kodi
   * \param size The size of the data array
   */
  virtual void AddData(unsigned int width,
                       unsigned int height,
                       const uint8_t* data,
                       size_t size) = 0;
};

} // namespace SMART_HOME
} // namespace KODI
