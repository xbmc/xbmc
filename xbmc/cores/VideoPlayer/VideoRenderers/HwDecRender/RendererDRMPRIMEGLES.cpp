/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererDRMPRIMEGLES.h"

#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "windowing/gbm/WinSystemGbmGLESContext.h"

using namespace KODI::WINDOWING::GBM;

CRendererDRMPRIMEGLES::~CRendererDRMPRIMEGLES()
{
  for (int i = 0; i < NUM_BUFFERS; ++i)
    DeleteTexture(i);
}

CBaseRenderer* CRendererDRMPRIMEGLES::Create(CVideoBuffer* buffer)
{
  if (buffer && dynamic_cast<CVideoBufferDRMPRIME*>(buffer))
    return new CRendererDRMPRIMEGLES();

  return nullptr;
}

void CRendererDRMPRIMEGLES::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("drm_prime_gles", CRendererDRMPRIMEGLES::Create);
}

bool CRendererDRMPRIMEGLES::Configure(const VideoPicture &picture, float fps, unsigned int orientation)
{
  CWinSystemGbmGLESContext* winSystem = dynamic_cast<CWinSystemGbmGLESContext*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return false;

  for (auto &texture : m_DRMPRIMETextures)
    texture.Init(winSystem->GetEGLDisplay());

  return CLinuxRendererGLESBase::Configure(picture, fps, orientation);
}

void CRendererDRMPRIMEGLES::ReleaseBuffer(int index)
{
  m_DRMPRIMETextures[index].Unmap();
  CLinuxRendererGLESBase::ReleaseBuffer(index);
}

bool CRendererDRMPRIMEGLES::CreateTexture(int index)
{
  CPictureBuffer &buf = m_buffers[index];
  YuvImage &im = buf.image;
  CYuvPlane &plane = buf.fields[0][0];

  DeleteTexture(index);

  im = {};
  plane = {};

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;

  plane.id = 1;

  return true;
}

void CRendererDRMPRIMEGLES::DeleteTexture(int index)
{
  ReleaseBuffer(index);

  CPictureBuffer &buf = m_buffers[index];
  buf.fields[0][0].id = 0;
}

bool CRendererDRMPRIMEGLES::UploadTexture(int index)
{
  CPictureBuffer &buf = m_buffers[index];

  CVideoBufferDRMPRIME *buffer = dynamic_cast<CVideoBufferDRMPRIME*>(buf.videoBuffer);

  if (!buffer)
  {
    CLog::Log(LOGNOTICE, "CRendererDRMPRIMEGLES::%s - no buffer", __FUNCTION__);
    return false;
  }

  m_DRMPRIMETextures[index].Map(buffer);

  CYuvPlane &plane = buf.fields[0][0];

  auto size = m_DRMPRIMETextures[index].GetTextureSize();
  plane.texwidth  = size.Width();
  plane.texheight = size.Height();
  plane.pixpertex_x = 1;
  plane.pixpertex_y = 1;

  plane.id = m_DRMPRIMETextures[index].GetTexture();

  CalculateTextureSourceRects(index, 1);

  return true;
}

bool CRendererDRMPRIMEGLES::LoadShadersHook()
{
  CLog::Log(LOGNOTICE, "Using DRMPRIMEGLES render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_CUSTOM;
  return true;
}

bool CRendererDRMPRIMEGLES::RenderHook(int index)
{
  CRenderSystemGLES *renderSystem = dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  assert(renderSystem);

  CYuvPlane &plane = m_buffers[index].fields[0][0];

  glDisable(GL_DEPTH_TEST);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, plane.id);

  renderSystem->EnableGUIShader(SM_TEXTURE_RGBA_OES);

  GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of triangle strip
  GLuint vertexVBO;
  GLuint indexVBO;
  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
  } vertex[4];

  GLint vertLoc = renderSystem->GUIShaderGetPos();
  GLint loc = renderSystem->GUIShaderGetCoord0();

  for (unsigned int i = 0; i < 4; i++)
  {
    // Setup vertex position values
    vertex[i].x = m_rotatedDestCoords[i].x;
    vertex[i].y = m_rotatedDestCoords[i].y;
    vertex[i].z = 0.0f;
  }

  // Setup texture coordinates
  vertex[0].u1 = vertex[3].u1 = plane.rect.x1;
  vertex[0].v1 = vertex[1].v1 = plane.rect.y1;
  vertex[1].u1 = vertex[2].u1 = plane.rect.x2;
  vertex[2].v1 = vertex[3].v1 = plane.rect.y2;

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex)*4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex), reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glVertexAttribPointer(loc, 2, GL_FLOAT, 0, sizeof(PackedVertex), reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(loc);

  glGenBuffers(1, &indexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte)*4, idx, GL_STATIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(vertLoc);
  glDisableVertexAttribArray(loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &indexVBO);

  renderSystem->DisableGUIShader();

  glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

  return true;
}
