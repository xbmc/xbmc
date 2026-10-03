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

/*!
 * \brief The rendering context a game client asked for
 *
 * A rendering-system-agnostic description of the client's request, so the
 * buffer layer doesn't have to reach into the Game API for it.
 */
struct HwContextProperties
{
  //! \brief Request a core profile rather than a compatibility profile
  bool coreProfile{true};

  //! \brief Request an OpenGL ES context rather than desktop OpenGL
  bool embedded{false};

  //! \brief Minimum context version, or 0 to let the driver decide
  unsigned int versionMajor{0};
  unsigned int versionMinor{0};

  //! \brief Require a debug rendering context
  bool debugContext{false};

  //! \brief The core needs a depth attachment on the framebuffer
  bool depth{false};

  //! \brief The core needs a stencil attachment on the framebuffer
  bool stencil{false};

  /*!
   * \brief The client renders with OpenGL's bottom-left origin
   *
   * Bottom-left images are flipped vertically when sampled in GUI coordinates.
   */
  bool bottomLeftOrigin{true};
};

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

  /*!
   * \brief Whether this pool can give a client a framebuffer to render into
   *
   * False for every pool that holds frames a client has drawn elsewhere. Only
   * a pool that owns a rendering context can answer otherwise, and a build
   * without one has no pool that does.
   */
  virtual bool SupportsHardwareRendering() const { return false; }

  /*!
   * \brief Create the resources tied to the rendering context
   *
   * Create an unbound context before notifying the client that it is ready.
   * BeginClientFrame() binds it on the thread making the client call.
   *
   * \param properties The context the client asked for
   *
   * \return True if the context is ready, or the pool has none to create
   */
  virtual bool CreateContext(const HwContextProperties& properties) { return true; }

  /*!
   * \brief Make the client's rendering context current on this thread
   *
   * Bracket every call into a hardware-rendering client with this and
   * EndClientFrame(). Which thread the client renders on is its own business:
   * some open their stream from the game loop, others during load on Kodi's
   * rendering thread. A context binding is per-thread, so implementations must
   * restore whatever the thread had, or Kodi loses the surface it presents
   * with. Calls nest. Only a successful begin must be paired with an end.
   */
  virtual bool BeginClientFrame() { return true; }

  /*!
   * \brief Give this thread back the binding it had before BeginClientFrame()
   */
  virtual void EndClientFrame() {}

  /*!
   * \brief Take a copy of the frame a hardware-rendering client has just drawn
   *
   * A client draws into the same surface every frame, and clients cache it
   * rather than asking for it again, so it cannot be swapped out from under
   * them. Publishing that surface directly means the rendering thread samples
   * it while the next frame is being drawn into it, and what comes out missing
   * is whatever the game draws last.
   *
   * Called on the client's thread, between its frames, so the copy is of a
   * whole frame. Pools that do not render in hardware have nothing to copy.
   *
   * \return An acquired buffer holding the copy, or nullptr to drop the frame
   */
  virtual IRenderBuffer* CaptureClientFrame(IRenderBuffer* clientBuffer,
                                            unsigned int width,
                                            unsigned int height)
  {
    return nullptr;
  }

  /*!
   * \brief Release resources tied to the rendering context
   *
   * This function is called when the render context is being destroyed.
   * Implementations should free any context-specific resources so that the
   * pool can be safely recreated.
   *
   * Implementations must bind the owning context and restore the caller's binding.
   */
  virtual void DestroyContext() {}

  /*!
   * \brief Submit outstanding GUI sampling before recycling or destroying captures
   */
  virtual void FlushRendered() {}
};
} // namespace RETRO
} // namespace KODI
