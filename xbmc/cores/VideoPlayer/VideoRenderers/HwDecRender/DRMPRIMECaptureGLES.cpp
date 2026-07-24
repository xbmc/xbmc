/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMPRIMECaptureGLES.h"

#include "DRMPRIMEEGL.h"
#include "RendererDRMPRIMEGLES.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "rendering/MatrixGL.h"
#include "rendering/capture/CaptureReadback.h"
#include "rendering/capture/CaptureTypes.h"
#include "rendering/gles/RenderSystemGLES.h"

bool CaptureDRMPRIMEVideo(CVideoBufferDRMPRIME* buffer,
                          const KODI::RENDERING::CAPTURE::CaptureSpec& spec,
                          KODI::RENDERING::CAPTURE::CaptureResult& result)
{
#if HAS_GLES != 3
  // the capture FBO uses GLES 3 sized formats (GL_RGB10_A2 for the HDR case)
  (void)buffer;
  (void)spec;
  (void)result;
  return false;
#else
  using namespace KODI::RENDERING::CAPTURE;

  auto* renderSystem = dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  if (!renderSystem)
    return false;

  // the capture runs with the GL context current, so the display is at hand
  CDRMPRIMETexture texture;
  texture.Init(eglGetCurrentDisplay());
  if (!texture.Map(buffer))
    return false;

  const VideoPicture& picture = buffer->GetPicture();
  const unsigned int width = spec.width ? spec.width : buffer->GetWidth();
  const unsigned int height = spec.height ? spec.height : buffer->GetHeight();
  const bool highDepth = spec.format == CaptureFormat::NATIVE && picture.colorBits > 8;

  GLint prevFbo = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
  GLint prevTexture = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexture);
  GLint prevViewport[4] = {};
  glGetIntegerv(GL_VIEWPORT, prevViewport);
  const GLboolean prevScissor = glIsEnabled(GL_SCISSOR_TEST);

  GLuint fbo = 0;
  GLuint fboTexture = 0;
  glGenTextures(1, &fboTexture);
  glBindTexture(GL_TEXTURE_2D, fboTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, highDepth ? GL_RGB10_A2 : GL_RGBA8, static_cast<GLsizei>(width),
               static_cast<GLsizei>(height), 0, GL_RGBA,
               highDepth ? GL_UNSIGNED_INT_2_10_10_10_REV : GL_UNSIGNED_BYTE, nullptr);
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTexture));

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

  // no separate ES2 gate: the sized formats fail completeness on an ES2 context
  bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  if (ok)
  {
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glDisable(GL_SCISSOR_TEST); // no stale GUI clip on the capture draw

    glMatrixModview.Push();
    glMatrixModview->LoadIdentity();
    glMatrixModview.Load();
    glMatrixProject.Push();
    glMatrixProject->LoadIdentity();
    glMatrixProject->Ortho2D(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
    glMatrixProject.Load();

    // source top (texcoord v=0) placed at the FBO bottom, so a bottom-up
    // glReadPixels yields top-down rows and the readback needs no flip
    const CPoint dest[4] = {{0.0f, 0.0f},
                            {static_cast<float>(width), 0.0f},
                            {static_cast<float>(width), static_cast<float>(height)},
                            {0.0f, static_cast<float>(height)}};
    CRendererDRMPRIMEGLES::DrawTexture(*renderSystem, texture.GetTexture(), dest);

    glMatrixProject.PopLoad();
    glMatrixModview.PopLoad();

    ReadbackBuffer readback;
    ok = ReadFramebufferRegion(0, 0, width, height, highDepth ? picture.colorBits : 8, false,
                               readback);
    if (ok)
    {
      result.pixels = std::move(readback.pixels);
      result.width = readback.width;
      result.height = readback.height;
      result.stride = readback.stride;
      result.bitDepth = readback.bitDepth;
      result.content = CaptureContent::VIDEO;
      // OES sampler emits full-range RGB; tag the video's own primaries/transfer
      result.color.primaries = picture.color_primaries;
      result.color.transfer = picture.color_transfer;
      result.color.range = AVCOL_RANGE_JPEG;
      result.hasDisplayMetadata = picture.hasDisplayMetadata;
      result.displayMetadata = picture.displayMetadata;
      result.hasLightMetadata = picture.hasLightMetadata;
      result.lightMetadata = picture.lightMetadata;
    }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
  glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
  if (prevScissor)
    glEnable(GL_SCISSOR_TEST);
  glDeleteFramebuffers(1, &fbo);
  glDeleteTextures(1, &fboTexture);
  texture.Unmap();
  return ok;
#endif
}
