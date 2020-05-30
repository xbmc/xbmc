/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RetroPlayerStreamTypes.h"

namespace KODI
{
namespace RETRO
{

struct StreamProperties
{
};

struct StreamBuffer
{
};

struct StreamPacket
{
};

class IRetroPlayerStream
{
public:
  virtual ~IRetroPlayerStream() = default;

  /*!
   * \brief Open a stream
   *
   * \return True if the stream was opened, false otherwise
   */
  virtual bool OpenStream(const StreamProperties& properties) = 0;

  /*!
   * \brief Get a buffer for zero-copy stream data
   *
   * \param width The framebuffer width, or 0 for no width specified
   * \param height The framebuffer height, or 0 for no height specified
   * \param[out] buffer The buffer, or unmodified if false is returned
   *
   * \return True if a buffer was returned, false otherwise
   */
  virtual bool GetStreamBuffer(unsigned int width, unsigned int height, StreamBuffer& buffer) = 0;

  /*!
   * \brief Add a data packet to a stream
   *
   * \param packet The data packet
   */
  virtual void AddStreamData(const StreamPacket& packet) = 0;

  /*!
   * \brief Close the stream
   */
  virtual void CloseStream() = 0;
};

} // namespace RETRO
} // namespace KODI
