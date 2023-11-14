/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

struct game_stream_buffer;
struct game_stream_packet;
struct game_stream_properties;

namespace KODI
{
namespace RETRO
{
class IRetroPlayerStream;
}

namespace GAME
{

/*!
 * \ingroup games
 */
class IGameClientStream
{
public:
  virtual ~IGameClientStream() = default;

  /*!
   * \brief Open the stream
   *
   * \param stream The RetroPlayer resource to take ownership of
   *
   * \return True if the stream was opened, false otherwise
   */
  virtual bool OpenStream(RETRO::IRetroPlayerStream* stream,
                          const game_stream_properties& properties) = 0;

  /*!
   * \brief Release the RetroPlayer stream resource
   */
  virtual void CloseStream() = 0;

  /*!
   * \brief Get a buffer for zero-copy stream data
   *
   * \param width The framebuffer width, or 0 for no width specified
   * \param height The framebuffer height, or 0 for no height specified
   * \param[out] buffer The buffer, or unmodified if false is returned
   *
   * If this returns true, buffer must be freed using ReleaseBuffer().
   *
   * \return True if buffer was set, false otherwise
   */
  virtual bool GetBuffer(unsigned int width, unsigned int height, game_stream_buffer& buffer)
  {
    return false;
  }

  /*!
   * \brief Free an allocated buffer
   *
   * \param buffer The buffer returned from GetBuffer()
   */
  virtual void ReleaseBuffer(game_stream_buffer& buffer) {}

  /*!
   * \brief Add a data packet to a stream
   *
   * \param packet The data packet
   */
  virtual void AddData(const game_stream_packet& packet) = 0;
};

} // namespace GAME
} // namespace KODI
