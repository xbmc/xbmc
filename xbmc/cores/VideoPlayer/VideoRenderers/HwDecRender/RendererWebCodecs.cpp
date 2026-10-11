/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererWebCodecs.h"

#include "../RenderFactory.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecWebCodecs.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

namespace
{
constexpr unsigned int RENDER_BUFFERS = 4;
} // namespace

CRendererWebCodecs::CRendererWebCodecs()
{
  m_textureTarget = GL_TEXTURE_2D;
}

CRendererWebCodecs::~CRendererWebCodecs()
{
  for (int i = 0; i < NUM_BUFFERS; ++i)
    DeleteTexture(i);
}

CBaseRenderer* CRendererWebCodecs::Create(CVideoBuffer* buffer)
{
  if (buffer && dynamic_cast<CVideoBufferWebCodecs*>(buffer))
    return new CRendererWebCodecs();
  return nullptr;
}

bool CRendererWebCodecs::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("webcodecs", CRendererWebCodecs::Create);
  return true;
}

CRenderInfo CRendererWebCodecs::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = RENDER_BUFFERS;
  return info;
}

// FFmpeg takes over with sysmem pictures when the decoder keeps failing.
bool CRendererWebCodecs::ConfigChanged(const VideoPicture& picture)
{
  return !dynamic_cast<CVideoBufferWebCodecs*>(picture.videoBuffer) ||
         CLinuxRendererGLES::ConfigChanged(picture);
}

bool CRendererWebCodecs::Supports(ERENDERFEATURE feature) const
{
  // The browser converts the frame to the canvas colour space before Kodi
  // sees it.
  if (feature == RENDERFEATURE_TONEMAP)
    return false;
  return CLinuxRendererGLES::Supports(feature);
}

bool CRendererWebCodecs::Supports(ESCALINGMETHOD method) const
{
  return method == VS_SCALINGMETHOD_NEAREST || method == VS_SCALINGMETHOD_LINEAR ||
         method == VS_SCALINGMETHOD_AUTO;
}

bool CRendererWebCodecs::LoadShadersHook()
{
  CLog::Log(LOGINFO, "GL: Using WebCodecs render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_CUSTOM;
  return true;
}

// The browser imports the frame as an RGB texture.
EShaderFormat CRendererWebCodecs::GetShaderFormat()
{
  return SHADER_RGB;
}

GLenum CRendererWebCodecs::TextureFilter() const
{
  return m_videoSettings.m_ScalingMethod == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR;
}

// UpdateVideoFilter() refilters RENDER_GLSL textures only, so a method changed
// during playback is applied here.
void CRendererWebCodecs::ApplyTextureFilter()
{
  const GLenum filter = TextureFilter();
  for (int i = 0; i < m_NumYUVBuffers; ++i)
  {
    const GLuint texture = m_buffers[i].fields[FIELD_FULL][0].id;
    if (!texture)
      continue;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  m_appliedScalingMethod = m_videoSettings.m_ScalingMethod;
}

// One texture per buffer and no storage: the upload's texImage2D specifies
// level 0 at the frame's size each time.
bool CRendererWebCodecs::CreateTexture(int index)
{
  CPictureBuffer& buf = m_buffers[index];
  buf.image.width = m_sourceWidth;
  buf.image.height = m_sourceHeight;
  buf.image.cshift_x = 0;
  buf.image.cshift_y = 0;

  for (int field = 0; field < MAX_FIELDS; ++field)
  {
    CYuvPlane& plane = buf.fields[field][0];
    plane.texwidth = m_sourceWidth;
    plane.texheight = m_sourceHeight;
    plane.pixpertex_x = 1;
    plane.pixpertex_y = 1;
  }

  CYuvPlane& plane = buf.fields[FIELD_FULL][0];
  if (!plane.id)
    glGenTextures(1, &plane.id);

  const GLenum filter = TextureFilter();
  glBindTexture(GL_TEXTURE_2D, plane.id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
  m_appliedScalingMethod = m_videoSettings.m_ScalingMethod;
  return true;
}

void CRendererWebCodecs::DeleteTexture(int index)
{
  ReleaseBuffer(index);

  CPictureBuffer& buf = m_buffers[index];
  CYuvPlane& plane = buf.fields[FIELD_FULL][0];
  if (plane.id)
  {
    glDeleteTextures(1, &plane.id);
    plane.id = 0;
  }
  buf.loaded = false;
}

// Queues the import of the frame into this buffer's texture; `loaded` keeps the
// texture for repeated and paused frames.
bool CRendererWebCodecs::UploadTexture(int index)
{
  CPictureBuffer& buf = m_buffers[index];
  if (buf.loaded)
    return true;

  auto* videoBuffer = dynamic_cast<CVideoBufferWebCodecs*>(buf.videoBuffer);
  const GLuint texture = buf.fields[FIELD_FULL][0].id;
  if (!videoBuffer || !texture)
    return false;

  webcodecs_upload_frame(videoBuffer->GetDecoderHandle(), videoBuffer->GetSequence(), texture);
  buf.loaded = true;
  return true;
}

void CRendererWebCodecs::AddVideoPicture(const VideoPicture& picture, int index)
{
  CLinuxRendererGLES::AddVideoPicture(picture, index);
  m_pendingUploads.fetch_or(1u << index, std::memory_order_release);
}

// The Samsung decoder reuses its oldest output buffer for a new frame while a
// VideoFrame still refers to it, so a frame is imported as soon as it is
// queued; the texture keeps the pixels until the picture is due.
void CRendererWebCodecs::UploadPendingFrames()
{
  uint32_t pending = m_pendingUploads.exchange(0, std::memory_order_acquire);
  for (int index = 0; pending != 0; ++index, pending >>= 1)
  {
    if (!(pending & 1u))
      continue;

    const CPictureBuffer& buf = m_buffers[index];
    if (!buf.videoBuffer || buf.loaded)
      continue;

    // No texture yet: ValidateRenderTarget creates them on the first pass.
    if (!buf.fields[FIELD_FULL][0].id)
    {
      m_pendingUploads.fetch_or(1u << index, std::memory_order_relaxed);
      continue;
    }

    UploadTexture(index);
  }
}

void CRendererWebCodecs::RenderUpdate(
    int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  UploadPendingFrames();
  m_alpha = alpha;
  CLinuxRendererGLES::RenderUpdate(index, index2, clear, flags, alpha);
}

bool CRendererWebCodecs::RenderHook(int index)
{
  if (m_appliedScalingMethod != m_videoSettings.m_ScalingMethod)
    ApplyTextureFilter();

  CalculateTextureSourceRects(index, 1);
  const CYuvPlane& plane = m_buffers[index].fields[FIELD_FULL][0];

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, plane.id);

  m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE_RGBA);
  glUniform1f(m_renderSystem->GUIShaderGetContrast(), m_videoSettings.m_Contrast * 0.02f);
  glUniform1f(m_renderSystem->GUIShaderGetBrightness(),
              m_videoSettings.m_Brightness * 0.01f - 0.5f);
  glUniform1f(m_renderSystem->GUIShaderGetDepth(), -1.0f);

  // The RGBA shader has no opacity input, unlike the YUV ones, so the blend
  // stage applies it; the base class has enabled blending when alpha < 255.
  if (m_alpha < 255)
  {
    glBlendColor(0.0f, 0.0f, 0.0f, m_alpha / 255.0f);
    glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
  }

  // m_rotatedDestCoords holds the corners top left, top right, bottom right,
  // bottom left. Row 0 of the texture is the top of the frame, as with Kodi's
  // own uploads.
  m_renderSystem->DrawGUIQuad(m_rotatedDestCoords[0], m_rotatedDestCoords[1],
                              m_rotatedDestCoords[3], &plane.rect);

  if (m_alpha < 255)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  m_renderSystem->DisableGUIShader();
  glBindTexture(GL_TEXTURE_2D, 0);
  VerifyGLState();
  return true;
}
