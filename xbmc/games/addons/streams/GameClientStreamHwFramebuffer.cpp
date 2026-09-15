/*
 *  Copyright (C) 2018-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientStreamHwFramebuffer.h"

#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "cores/RetroPlayer/streams/RetroPlayerRendering.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

CGameClientStreamHwFramebuffer::CGameClientStreamHwFramebuffer(
    IHwFramebufferCallback& callback, const game_hw_rendering_properties& hwProperties)
  : m_callback(callback),
    m_hwProperties(std::make_unique<const game_hw_rendering_properties>(hwProperties))
{
}

bool CGameClientStreamHwFramebuffer::OpenStream(RETRO::IRetroPlayerStream* stream,
                                                const game_stream_properties& properties)
{
  RETRO::CRetroPlayerRendering* renderingStream =
      dynamic_cast<RETRO::CRetroPlayerRendering*>(stream);
  if (renderingStream == nullptr)
  {
    CLog::Log(LOGERROR, "GAME: RetroPlayer stream is not a rendering stream");
    return false;
  }

  std::unique_ptr<RETRO::HwFramebufferProperties> hwProperties =
      TranslateProperties(*m_hwProperties, properties.hw_framebuffer);

  if (stream->OpenStream(*hwProperties))
    m_stream = stream;

  if (m_stream == nullptr)
    return false;

  m_hwContextDestroyed = false;
  if (!m_callback.HardwareContextReset())
  {
    CloseStream();
    return false;
  }

  return true;
}

void CGameClientStreamHwFramebuffer::DestroyHwContext()
{
  if (m_stream == nullptr || m_hwContextDestroyed)
    return;

  m_hwContextDestroyed = true;

  // Let the client release its GPU resources while its context is still
  // current. The context itself outlives this call, so a client that rebuilds
  // something on the way out still has one to do it in.
  m_callback.HardwareContextDestroy();
}

void CGameClientStreamHwFramebuffer::CloseStream()
{
  if (m_stream != nullptr)
  {
    // Normally already done, from before the game was unloaded
    DestroyHwContext();

    m_stream->CloseStream();
    m_stream = nullptr;
  }
}

bool CGameClientStreamHwFramebuffer::GetBuffer(unsigned int width,
                                               unsigned int height,
                                               game_stream_buffer& buffer)
{
  if (buffer.type != GAME_STREAM_HW_FRAMEBUFFER)
    return false;

  if (m_stream != nullptr)
  {
    RETRO::HwFramebufferBuffer hwFramebufferBuffer;
    if (m_stream->GetStreamBuffer(width, height,
                                  static_cast<RETRO::StreamBuffer&>(hwFramebufferBuffer)))
    {
      buffer.hw_framebuffer.framebuffer = hwFramebufferBuffer.framebuffer;
      return true;
    }
  }

  return false;
}

void CGameClientStreamHwFramebuffer::AddData(const game_stream_packet& packet)
{
  if (packet.type != GAME_STREAM_HW_FRAMEBUFFER)
    return;

  if (m_stream != nullptr)
  {
    const game_stream_hw_framebuffer_packet& hwFramebuffer = packet.hw_framebuffer;

    RETRO::HwFramebufferPacket hwFramebufferPacket{hwFramebuffer.framebuffer, hwFramebuffer.width,
                                                   hwFramebuffer.height};
    m_stream->AddStreamData(static_cast<const RETRO::StreamPacket&>(hwFramebufferPacket));
  }
}

void CGameClientStreamHwFramebuffer::LogHwProperties(
    const game_hw_rendering_properties& hwProperties)
{
  const std::string strContextType = CGameClientStreamHwFramebuffer::GetContextName(
      hwProperties.context_type, hwProperties.version_major, hwProperties.version_minor);

  CLog::Log(LOGINFO, "Enabling hardware rendering for {}", strContextType);
  CLog::Log(LOGDEBUG, "  depth: {}", hwProperties.depth ? "true" : "false");
  CLog::Log(LOGDEBUG, "  stencil: {}", hwProperties.stencil ? "true" : "false");
  CLog::Log(LOGDEBUG, "  bottomLeftOrigin: {}", hwProperties.bottom_left_origin ? "true" : "false");
  CLog::Log(LOGDEBUG, "  cacheContext: {}", hwProperties.cache_context ? "true" : "false");
  CLog::Log(LOGDEBUG, "  debugContext: {}", hwProperties.debug_context ? "true" : "false");
}

std::string CGameClientStreamHwFramebuffer::GetContextName(GAME_HW_CONTEXT_TYPE contextType,
                                                           unsigned int versionMajor,
                                                           unsigned int versionMinor)
{
  std::string strContextType;

  switch (contextType)
  {
    case GAME_HW_CONTEXT_OPENGL:
      strContextType = "OpenGL 2.x";
      break;
    case GAME_HW_CONTEXT_OPENGLES2:
      strContextType = "OpenGLES 2.0";
      break;
    case GAME_HW_CONTEXT_OPENGL_CORE:
      strContextType = StringUtils::Format("OpenGL {}.{}", versionMajor, versionMinor);
      break;
    case GAME_HW_CONTEXT_OPENGLES3:
      strContextType = "OpenGLES 3.0";
      break;
    case GAME_HW_CONTEXT_OPENGLES_VERSION:
      strContextType = StringUtils::Format("OpenGLES {}.{}", versionMajor, versionMinor);
      break;
    case GAME_HW_CONTEXT_VULKAN:
      strContextType = "Vulkan";
      break;
    default:
      strContextType = "Unknown";
      break;
  }

  return strContextType;
}

std::unique_ptr<RETRO::HwFramebufferProperties> CGameClientStreamHwFramebuffer::TranslateProperties(
    const game_hw_rendering_properties& hwProperties,
    const game_stream_hw_framebuffer_properties& streamProperties)
{
  return std::make_unique<RETRO::HwFramebufferProperties>(
      hwProperties.context_type, hwProperties.depth, hwProperties.stencil,
      hwProperties.bottom_left_origin, hwProperties.version_major, hwProperties.version_minor,
      hwProperties.cache_context, hwProperties.debug_context, streamProperties.max_width,
      streamProperties.max_height);
}
