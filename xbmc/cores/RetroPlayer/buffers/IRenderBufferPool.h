/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/RetroPlayerTypes.h"

extern "C"
{
#include <libavutil/pixfmt.h>
}

#include <memory>
#include <stdint.h>

namespace KODI
{
namespace RETRO
{
class CRenderBufferManager;
class CRenderVideoSettings;
class CRPBaseRenderer;
class IRenderBuffer;

class IRenderBufferPool : public std::enable_shared_from_this<IRenderBufferPool>
{
public:
  virtual ~IRenderBufferPool() = default;

  virtual void RegisterRenderer(CRPBaseRenderer* renderer) = 0;
  virtual void UnregisterRenderer(CRPBaseRenderer* renderer) = 0;
  virtual bool HasVisibleRenderer() const = 0;

  virtual bool Configure(AVPixelFormat format) = 0;

  virtual bool IsConfigured() const = 0;

  virtual bool IsCompatible(const CRenderVideoSettings& renderSettings) const = 0;

  /*!
   * \brief Get a free buffer from the pool, sets ref count to 1
   *
   * \param width The horizontal pixel count of the buffer
   * \param height The vertical pixel could of the buffer
   *
   * \return The allocated buffer, or nullptr on failure
   */
  virtual IRenderBuffer* GetBuffer(unsigned int width, unsigned int height) = 0;

  /*!
   * \brief Called by buffer when ref count goes to zero
   *
   * \param buffer A fully dereferenced buffer
   */
  virtual void Return(IRenderBuffer* buffer) = 0;

  virtual void Prime(unsigned int width, unsigned int height) = 0;

  virtual void Flush() = 0;

  virtual DataAccess GetMemoryAccess() const { return DataAccess::READ_WRITE; }
  virtual DataAlignment GetMemoryAlignment() const { return DataAlignment::DATA_UNALIGNED; }

  /*!
   * \brief Call in GetBuffer() before returning buffer to caller
   */
  virtual std::shared_ptr<IRenderBufferPool> GetPtr() { return shared_from_this(); }
};
} // namespace RETRO
} // namespace KODI
