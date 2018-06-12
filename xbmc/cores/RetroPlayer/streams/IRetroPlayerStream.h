/*
 *      Copyright (C) 2018 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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

}
}
