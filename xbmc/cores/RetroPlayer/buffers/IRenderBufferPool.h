/*
 *      Copyright (C) 2017 Team Kodi
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

extern "C" {
#include "libavutil/pixfmt.h"
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

    virtual void RegisterRenderer(CRPBaseRenderer *renderer) = 0;
    virtual void UnregisterRenderer(CRPBaseRenderer *renderer) = 0;
    virtual bool HasVisibleRenderer() const = 0;

    virtual bool Configure(AVPixelFormat format) = 0;

    virtual bool IsConfigured() const = 0;

    virtual bool IsCompatible(const CRenderVideoSettings &renderSettings) const = 0;

    /*!
     * \brief Get a free buffer from the pool, sets ref count to 1
     *
     * \param width The horizontal pixel count of the buffer
     * \param height The vertical pixel could of the buffer
     *
     * \return The allocated buffer, or nullptr on failure
     */
    virtual IRenderBuffer *GetBuffer(unsigned int width, unsigned int height) = 0;

    /*!
     * \brief Called by buffer when ref count goes to zero
     *
     * \param buffer A fully dereferenced buffer
     */
    virtual void Return(IRenderBuffer *buffer) = 0;

    virtual void Prime(unsigned int width, unsigned int height) = 0;

    virtual void Flush() = 0;

    /*!
     * \brief Call in GetBuffer() before returning buffer to caller
     */
    virtual std::shared_ptr<IRenderBufferPool> GetPtr() { return shared_from_this(); }
  };
}
}
