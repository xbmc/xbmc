/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMPRIMEEGL.h"

#include "utils/log.h"

#include <memory>

using namespace DRMPRIME;

namespace
{

int GetEGLColorSpace(const VideoPicture& picture)
{
  switch (picture.color_space)
  {
    case AVCOL_SPC_BT2020_CL:
    case AVCOL_SPC_BT2020_NCL:
      return EGL_ITU_REC2020_EXT;
    case AVCOL_SPC_SMPTE170M:
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_FCC:
      return EGL_ITU_REC601_EXT;
    case AVCOL_SPC_BT709:
      return EGL_ITU_REC709_EXT;
    case AVCOL_SPC_RESERVED:
    case AVCOL_SPC_UNSPECIFIED:
    default:
      if (picture.iWidth > 1024 || picture.iHeight >= 600)
        return EGL_ITU_REC709_EXT;
      else
        return EGL_ITU_REC601_EXT;
  }
}

int GetEGLColorRange(const VideoPicture& picture)
{
  if (picture.color_range)
    return EGL_YUV_FULL_RANGE_EXT;

  return EGL_YUV_NARROW_RANGE_EXT;
}

} // namespace

CDRMPRIMETexture::~CDRMPRIMETexture()
{
  glDeleteTextures(1, &m_texture);
}

void CDRMPRIMETexture::Init(EGLDisplay eglDisplay)
{
  m_eglImage = std::make_unique<CEGLImage>(eglDisplay);
}

bool CDRMPRIMETexture::Map(CVideoBufferDRMPRIME* buffer)
{
  if (m_primebuffer)
    return true;

  if (!buffer->AcquireDescriptor())
  {
    CLog::Log(LOGERROR, "CDRMPRIMETexture::{} - failed to acquire descriptor", __FUNCTION__);
    return false;
  }

  m_texWidth = buffer->GetWidth();
  m_texHeight = buffer->GetHeight();

  AVDRMFrameDescriptor* descriptor = buffer->GetDescriptor();
  if (descriptor && descriptor->nb_layers)
  {
    AVDRMLayerDescriptor* layer = &descriptor->layers[0];

    std::array<CEGLImage::EglPlane, CEGLImage::MAX_NUM_PLANES> planes;

    for (int i = 0; i < layer->nb_planes; i++)
    {
      planes[i].fd = descriptor->objects[layer->planes[i].object_index].fd;
      planes[i].modifier = descriptor->objects[layer->planes[i].object_index].format_modifier;
      planes[i].offset = layer->planes[i].offset;
      planes[i].pitch = layer->planes[i].pitch;
    }

    CEGLImage::EglAttrs attribs;

    attribs.width = m_texWidth;
    attribs.height = m_texHeight;
    attribs.format = layer->format;
    attribs.colorSpace = GetEGLColorSpace(buffer->GetPicture());
    attribs.colorRange = GetEGLColorRange(buffer->GetPicture());
    attribs.planes = planes;

    if (!m_eglImage->CreateImage(attribs))
    {
      buffer->ReleaseDescriptor();
      return false;
    }

    if (!glIsTexture(m_texture))
      glGenTextures(1, &m_texture);
    glBindTexture(m_textureTarget, m_texture);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_eglImage->UploadImage(m_textureTarget);
    glBindTexture(m_textureTarget, 0);
  }

  m_primebuffer = buffer;
  m_primebuffer->Acquire();

  return true;
}

void CDRMPRIMETexture::Unmap()
{
  if (!m_primebuffer)
    return;

  m_eglImage->DestroyImage();

  m_primebuffer->ReleaseDescriptor();

  m_primebuffer->Release();
  m_primebuffer = nullptr;
}

namespace
{

// Maps a source DRM fourcc to the per-plane import descriptors used by
// CDRMPRIMETextureYUV. planeWidthShift / planeHeightShift describe
// chroma subsampling relative to the full Y-plane resolution.
struct PlaneLayout
{
  int numPlanes;
  uint32_t planeFourcc[CDRMPRIMETextureYUV::MAX_PLANES];
  int planeWidthShift[CDRMPRIMETextureYUV::MAX_PLANES];
  int planeHeightShift[CDRMPRIMETextureYUV::MAX_PLANES];
};

bool GetPlaneLayout(uint32_t sourceFormat, PlaneLayout& layout)
{
  switch (sourceFormat)
  {
    case DRM_FORMAT_NV12:
      layout = {2, {DRM_FORMAT_R8, DRM_FORMAT_GR88, 0}, {0, 1, 0}, {0, 1, 0}};
      return true;
    case DRM_FORMAT_P010:
    case DRM_FORMAT_P012:
    case DRM_FORMAT_P016:
      layout = {2, {DRM_FORMAT_R16, DRM_FORMAT_GR1616, 0}, {0, 1, 0}, {0, 1, 0}};
      return true;
    case DRM_FORMAT_YUV420:
      layout = {3, {DRM_FORMAT_R8, DRM_FORMAT_R8, DRM_FORMAT_R8}, {0, 1, 1}, {0, 1, 1}};
      return true;
#if defined(DRM_FORMAT_S010)
    case DRM_FORMAT_S010:
#endif
#if defined(DRM_FORMAT_S012)
    case DRM_FORMAT_S012:
#endif
#if defined(DRM_FORMAT_S016)
    case DRM_FORMAT_S016:
#endif
#if defined(DRM_FORMAT_S010) || defined(DRM_FORMAT_S012) || defined(DRM_FORMAT_S016)
      layout = {3, {DRM_FORMAT_R16, DRM_FORMAT_R16, DRM_FORMAT_R16}, {0, 1, 1}, {0, 1, 1}};
      return true;
#endif
    default:
      return false;
  }
}

} // namespace

bool CDRMPRIMETextureYUV::SupportsFormat(uint32_t fourcc)
{
  PlaneLayout dummy;
  return GetPlaneLayout(fourcc, dummy);
}

// Cleanup pattern taken from CDRMPRIMETexture::~CDRMPRIMETexture (above),
// extended to the per-plane texture array.
CDRMPRIMETextureYUV::~CDRMPRIMETextureYUV()
{
  // Release any active mapping (EGL images, descriptor, buffer ref).
  Unmap();
  // Delete the GL texture objects that Map() may have generated.
  for (int i = 0; i < MAX_PLANES; i++)
  {
    if (m_textures[i])
      glDeleteTextures(1, &m_textures[i]);
  }
}

void CDRMPRIMETextureYUV::Init(EGLDisplay eglDisplay)
{
  m_eglDisplay = eglDisplay;
}

// Descriptor acquire/release lifecycle and the per-plane
// glIsTexture/glGenTextures + glBindTexture + glTexParameteri x4 +
// UploadImage + unbind sequence are taken from CDRMPRIMETexture::Map
// (above) and CVaapi2Texture::Map (VaapiEGL.cpp:438-560). What is new
// here is the per-plane DRM fourcc derivation (single-layer N-plane
// descriptor split into N separate EGL images) and the multi-plane
// loop. Single-layer-N-plane is the shape ffmpeg HW decoders and the
// SW-decode -> DMA path produce.
bool CDRMPRIMETextureYUV::Map(CVideoBufferDRMPRIME* buffer)
{
  if (m_primebuffer)
    return true;

  if (!buffer->AcquireDescriptor())
  {
    CLog::Log(LOGERROR, "CDRMPRIMETextureYUV::{} - failed to acquire descriptor", __FUNCTION__);
    return false;
  }

  AVDRMFrameDescriptor* descriptor = buffer->GetDescriptor();
  if (!descriptor || descriptor->nb_layers < 1)
  {
    buffer->ReleaseDescriptor();
    return false;
  }

  // Only the single-layer-multi-plane descriptor shape is supported.
  // ffmpeg HW decoders and the SW decode -> DMA path both produce this shape.
  if (descriptor->nb_layers != 1)
  {
    CLog::Log(LOGWARNING,
              "CDRMPRIMETextureYUV::{} - {} layers, only single-layer descriptors supported",
              __FUNCTION__, descriptor->nb_layers);
    buffer->ReleaseDescriptor();
    return false;
  }

  AVDRMLayerDescriptor* layer = &descriptor->layers[0];

  PlaneLayout planeLayout;
  if (!GetPlaneLayout(layer->format, planeLayout))
  {
    CLog::Log(LOGWARNING, "CDRMPRIMETextureYUV::{} - unsupported source fourcc {:#x}", __FUNCTION__,
              layer->format);
    buffer->ReleaseDescriptor();
    return false;
  }

  if (layer->nb_planes != planeLayout.numPlanes)
  {
    CLog::Log(LOGERROR,
              "CDRMPRIMETextureYUV::{} - layer has {} planes, expected {} for fourcc {:#x}",
              __FUNCTION__, layer->nb_planes, planeLayout.numPlanes, layer->format);
    buffer->ReleaseDescriptor();
    return false;
  }

  m_texWidth = buffer->GetWidth();
  m_texHeight = buffer->GetHeight();
  m_sourceFormat = layer->format;
  m_numPlanes = planeLayout.numPlanes;

  for (int i = 0; i < m_numPlanes; i++)
  {
    AVDRMPlaneDescriptor& plane = layer->planes[i];
    AVDRMObjectDescriptor& object = descriptor->objects[plane.object_index];

    CEGLImage::EglAttrs attribs{};
    attribs.width = m_texWidth >> planeLayout.planeWidthShift[i];
    attribs.height = m_texHeight >> planeLayout.planeHeightShift[i];
    attribs.format = planeLayout.planeFourcc[i];
    attribs.planes[0].fd = object.fd;
    attribs.planes[0].modifier = object.format_modifier;
    attribs.planes[0].offset = static_cast<int>(plane.offset);
    attribs.planes[0].pitch = static_cast<int>(plane.pitch);

    if (!m_eglImages[i])
      m_eglImages[i] = std::make_unique<CEGLImage>(m_eglDisplay);

    if (!m_eglImages[i]->CreateImage(attribs))
    {
      // Roll back any planes we already imported in this Map() call.
      for (int j = 0; j < i; j++)
        m_eglImages[j]->DestroyImage();
      m_numPlanes = 0;
      buffer->ReleaseDescriptor();
      return false;
    }

    if (!glIsTexture(m_textures[i]))
      glGenTextures(1, &m_textures[i]);
    glBindTexture(GL_TEXTURE_2D, m_textures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_eglImages[i]->UploadImage(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  m_primebuffer = buffer;
  m_primebuffer->Acquire();
  return true;
}

// DestroyImage + ReleaseDescriptor + buffer Release + null sequence is
// taken from CDRMPRIMETexture::Unmap (above), looped over planes.
void CDRMPRIMETextureYUV::Unmap()
{
  if (!m_primebuffer)
    return;

  for (int i = 0; i < m_numPlanes; i++)
  {
    if (m_eglImages[i])
      m_eglImages[i]->DestroyImage();
  }
  m_numPlanes = 0;

  m_primebuffer->ReleaseDescriptor();
  m_primebuffer->Release();
  m_primebuffer = nullptr;
}
