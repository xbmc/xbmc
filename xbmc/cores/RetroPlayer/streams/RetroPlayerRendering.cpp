/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayerRendering.h"

#include <algorithm>

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "cores/RetroPlayer/rendering/RenderTranslator.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRetroPlayerRendering::CRetroPlayerRendering(CRPRenderManager& renderManager,
                                             CRPProcessInfo& processInfo)
  : m_renderManager(renderManager),
    m_processInfo(processInfo)
{
  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Initializing rendering");
}

CRetroPlayerRendering::~CRetroPlayerRendering()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Deinitializing rendering");

  CloseStream();
}

bool CRetroPlayerRendering::OpenStream(const StreamProperties& properties)
{
  if (m_bOpen)
    return false;
  m_hwProperties = std::make_unique<HwFramebufferProperties>(
      static_cast<const HwFramebufferProperties&>(properties));

  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Opening rendering stream");

  if (m_hwProperties->maxWidth == 0 || m_hwProperties->maxHeight == 0)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDERING]: Client reported no maximum frame size ({}x{})",
              m_hwProperties->maxWidth, m_hwProperties->maxHeight);
    m_hwProperties.reset();
    return false;
  }

  // Create the context the core will render with, and make it current, before
  // the caller announces the context is ready. Cores build their GL objects in
  // context_reset(), so there has to be a context current by then, and it has
  // to be current on this thread -- the game loop thread -- because that is
  // where the core will run. A GL context can only be current on one thread.
  if (!m_renderManager.CreateContext(TranslateContextProperties(*m_hwProperties)))
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDERING]: Failed to create rendering context");
    m_hwProperties.reset();
    return false;
  }

  m_bOpen = true;

  // Configure and allocate the framebuffer now, at the largest size the client
  // says it will render. Clients typically ask for their framebuffer from
  // inside HwContextReset(), which the caller invokes as soon as this returns,
  // so it has to exist by then; handing back 0 there would send the client's
  // drawing to the default framebuffer for the rest of the session.
  if (!Configure(m_hwProperties->maxWidth, m_hwProperties->maxHeight))
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDERING]: Failed to configure rendering stream");

    // The context was created above, and CloseStream() will not run for a
    // stream that never opened, so it is released here
    m_renderManager.DestroyContext();
    m_hwProperties.reset();
    m_bOpen = false;
    return false;
  }

  return true;
}

void CRetroPlayerRendering::CloseStream()
{
  if (!m_bOpen)
    return;

  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Closing rendering stream");

  // Where the context is destroyed. The game loop deliberately leaves it alone
  // as it ends, because by then a hardware client's teardown calls would land
  // on Kodi's context instead of its own.
  m_renderManager.DestroyContext();
  m_renderManager.Deinitialize();

  m_width = 0;
  m_height = 0;
  m_hwProperties.reset();
  m_bOpen = false;
}

bool CRetroPlayerRendering::GetStreamBuffer(unsigned int width,
                                            unsigned int height,
                                            StreamBuffer& buffer)
{
  if (!m_bOpen || width == 0 || height == 0)
    return false;

  HwFramebufferBuffer& hwBuffer = static_cast<HwFramebufferBuffer&>(buffer);

  // Only ever grow. The framebuffer was allocated at the maximum the client
  // said it would render, and a client may cache the name it is given, so
  // reallocating it smaller for a frame that happens to be smaller would hand
  // back a different framebuffer or leave the cached one undersized.
  if (width > m_width || height > m_height)
  {
    if (!Configure(std::max(width, m_width), std::max(height, m_height)))
    {
      hwBuffer.framebuffer = 0;
      return false;
    }
  }

  hwBuffer.framebuffer = m_renderManager.GetCurrentFramebuffer(width, height);

  // Returning a framebuffer of 0 would send the core's drawing to the default
  // framebuffer, so report failure instead and let it try again next frame.
  return hwBuffer.framebuffer != 0;
}

HwContextProperties CRetroPlayerRendering::TranslateContextProperties(
    const HwFramebufferProperties& properties)
{
  HwContextProperties contextProperties;

  switch (properties.contextType)
  {
    case GAME_HW_CONTEXT_OPENGL:
      // Legacy OpenGL. The core expects fixed-function and the compatibility
      // profile; it will not run against a core profile.
      contextProperties.coreProfile = false;
      break;
    case GAME_HW_CONTEXT_OPENGL_CORE:
      contextProperties.coreProfile = true;
      contextProperties.versionMajor = properties.versionMajor;
      contextProperties.versionMinor = properties.versionMinor;
      break;
    case GAME_HW_CONTEXT_OPENGLES2:
      // Three, not two: the version a client asks for is a minimum, and the
      // frame is copied out with glBlitFramebuffer and a fence, neither of
      // which is in OpenGL ES 2.0. A client that asked for 2 runs on 3.
      contextProperties.embedded = true;
      contextProperties.versionMajor = 3;
      break;
    case GAME_HW_CONTEXT_OPENGLES3:
      contextProperties.embedded = true;
      contextProperties.versionMajor = 3;
      break;
    case GAME_HW_CONTEXT_OPENGLES_VERSION:
      contextProperties.embedded = true;
      contextProperties.versionMajor = std::max(3u, properties.versionMajor);
      contextProperties.versionMinor = properties.versionMajor < 3 ? 0 : properties.versionMinor;
      break;
    default:
      break;
  }

  contextProperties.depth = properties.depth;
  contextProperties.stencil = properties.stencil;
  contextProperties.bottomLeftOrigin = properties.bottomLeftOrigin;

  return contextProperties;
}

bool CRetroPlayerRendering::Configure(unsigned int width, unsigned int height)
{
  if (width == 0 || height == 0)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDERING]: Core asked for a {}x{} framebuffer", width,
              height);
    return false;
  }

  // Already configured at this size
  if (m_width == width && m_height == height)
    return true;

  // Hardware-rendered frames live in a GPU framebuffer and are never read back
  // to system memory, so they carry no pixel format. The FBO buffer pool keys
  // off this to tell itself apart from the software pools.
  const AVPixelFormat pixelFormat = AV_PIX_FMT_NONE;
  const float displayAspectRatio = 0.0f; // 0.0f means square pixels

  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Configuring rendering stream - width {}, height {}",
            width, height);

  if (!m_renderManager.Create(width, height))
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDERING]: Failed to create rendering resources");
    return false;
  }

  if (m_width == 0)
  {
    if (!m_renderManager.Configure(pixelFormat, width, height, displayAspectRatio, width, height))
      return false;
    m_processInfo.SetVideoPixelFormat(pixelFormat);
    m_processInfo.SetVideoDimensions(width, height);
  }

  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Render manager configured");

  m_width = width;
  m_height = height;

  return true;
}

void CRetroPlayerRendering::AddStreamData(const StreamPacket& packet)
{
  const HwFramebufferPacket& hwPacket = static_cast<const HwFramebufferPacket&>(packet);

  if (m_bOpen && hwPacket.width != 0 && hwPacket.height != 0 && hwPacket.width <= m_width &&
      hwPacket.height <= m_height &&
      hwPacket.framebuffer ==
          m_renderManager.GetCurrentFramebuffer(hwPacket.width, hwPacket.height))
    m_renderManager.RenderFrame(hwPacket.width, hwPacket.height);
}
