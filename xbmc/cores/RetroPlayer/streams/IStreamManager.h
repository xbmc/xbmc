/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RetroPlayerStreamTypes.h"
#include "cores/RetroPlayer/RetroPlayerTypes.h"

namespace KODI
{
namespace RETRO
{

class IStreamManager
{
public:
  virtual ~IStreamManager() = default;

  /*!
   * \brief Create a stream for gameplay data
   *
   * \param streamType The stream type
   *
   * \return A stream handle, or empty on failure
   */
  virtual StreamPtr CreateStream(StreamType streamType) = 0;

  /*!
   * \brief Free the specified stream
   *
   * \param stream The stream to close
   */
  virtual void CloseStream(StreamPtr stream) = 0;

  /*!
   * \brief Update the video frame rate reported by the game
   *
   * \param fps The new frame rate
   */
  virtual void SetVideoFps(float fps) = 0;

  /*!
   * \brief Get a symbol from the hardware context
   *
   * \param symbol The symbol's name
   *
   * \return A function pointer for the specified symbol
   */
  virtual HwProcedureAddress GetHwProcedureAddress(const char* symbol) = 0;

  /*!
   * \brief Whether a client can be given a framebuffer to render into
   */
  virtual bool HasHardwareRendering() const = 0;

  /*!
   * \brief Make a hardware-rendering client's context current on this thread
   *
   * Goes through the manager rather than a stream, because a client builds its
   * resources while its stream is still being opened, before there is a stream
   * to ask.
   */
  virtual bool BeginClientFrame() { return true; }

  /*!
   * \brief Give this thread back the binding it had
   */
  virtual void EndClientFrame() {}
};

} // namespace RETRO
} // namespace KODI
