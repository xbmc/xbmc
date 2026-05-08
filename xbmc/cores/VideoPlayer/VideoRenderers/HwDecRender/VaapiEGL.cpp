/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VaapiEGL.h"

#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#include "utils/EGLUtils.h"
#include "utils/log.h"

#include <drm_fourcc.h>
#include <va/va_drmcommon.h>

using namespace VAAPI;

void CVaapi2Texture::Init(InteropInfo& interop)
{
  m_interop = interop;
  m_hasPlaneModifiers = CEGLUtils::HasExtension(m_interop.eglDisplay, "EGL_EXT_image_dma_buf_import_modifiers");
}

bool CVaapi2Texture::Map(CVaapiRenderPicture* pic)
{
  if (m_vaapiPic)
    return true;

  m_vaapiPic = pic;
  m_vaapiPic->Acquire();

  // Pairs with the Acquire() above; nulling lets a retry see unmapped state.
  auto failMap = [this]()
  {
    m_vaapiPic->Release();
    m_vaapiPic = nullptr;
    return false;
  };

  VAStatus status;

  VADRMPRIMESurfaceDescriptor surface;

  status = vaExportSurfaceHandle(pic->vadsp, pic->procPic.videoSurface,
    VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
    VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS,
    &surface);

  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGWARNING, "CVaapi2Texture::Map: vaExportSurfaceHandle failed - Error: {} ({})",
              vaErrorStr(status), status);
    return failMap();
  }

  // Remember fds to close them later
  if (surface.num_objects > m_drmFDs.size())
  {
    failMap();
    throw std::logic_error("Too many fds returned by vaExportSurfaceHandle");
  }

  for (uint32_t object = 0; object < surface.num_objects; object++)
  {
    m_drmFDs[object].attach(surface.objects[object].fd);
  }

  status = vaSyncSurface(pic->vadsp, pic->procPic.videoSurface);
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "CVaapi2Texture::Map: vaSyncSurface - Error: {} ({})", vaErrorStr(status),
              status);
    return failMap();
  }

  m_textureSize.Set(pic->DVDPic.iWidth, pic->DVDPic.iHeight);

  for (uint32_t layerNo = 0; layerNo < surface.num_layers; layerNo++)
  {
    int plane = 0;
    auto const& layer = surface.layers[layerNo];
    if (layer.num_planes != 1)
    {
      CLog::Log(LOGDEBUG,
                "CVaapi2Texture::Map: DRM-exported layer has {} planes - only 1 supported",
                layer.num_planes);
      return failMap();
    }
    // Driver-supplied object_index is untrusted; reject OOB before indexing.
    if (layer.object_index[plane] >= surface.num_objects)
    {
      CLog::Log(LOGERROR,
                "CVaapi2Texture::Map: layer {} plane {} object_index {} >= num_objects {}", layerNo,
                plane, layer.object_index[plane], surface.num_objects);
      return failMap();
    }
    auto const& object = surface.objects[layer.object_index[plane]];

    MappedTexture* texture{};
    EGLint width{m_textureSize.Width()};
    EGLint height{m_textureSize.Height()};

    switch (surface.num_layers)
    {
      case 1:
        // Single-plane packed formats (YUY2, Y210/Y212/Y216). The DMA-BUF
        // carries Y/Cb/Y/Cr (or wider) interleaved; bind to m_y and leave
        // m_vu unused. The shader (XBMC_Y210 in gles_yuv2rgb_basic.frag)
        // decodes the packed layout.
        texture = &m_y;
        break;
      case 2:
        switch (layerNo)
        {
          case 0:
            texture = &m_y;
            break;
          case 1:
            texture = &m_vu;
            if (surface.fourcc == VA_FOURCC_NV12 || surface.fourcc == VA_FOURCC_P010 ||
                surface.fourcc == VA_FOURCC_P012 || surface.fourcc == VA_FOURCC_P016)
            {
              // Adjust w/h for 4:2:0 subsampling on UV plane
              width = (width + 1) >> 1;
              height = (height + 1) >> 1;
            }
            break;
          default:
            failMap();
            throw std::logic_error("Impossible layer number");
        }
        break;
      default:
        CLog::Log(LOGDEBUG,
                  "CVaapi2Texture::Map: DRM-exported surface {} layers - only 1 or 2 supported",
                  surface.num_layers);
        return failMap();
    }

    // Mesa Y210 / Y212 / Y216 import quirk: Mesa imports those DRM fourccs
    // as a two-plane internal layout (RG_UNORM16 luma + RGBA16 chroma)
    // whose chroma plane is only reachable via the samplerExternalOES NIR
    // lowering pass. sampler2D sees only the luma plane (RG with B=0 and
    // A=1), losing chroma. Re-import the same DMA-BUF as a flat
    // ABGR16161616 texture at half width so sampler2D returns the four
    // 16-bit slots Y0 / Cb / Y1 / Cr directly.
    EGLint eglFourcc = static_cast<EGLint>(layer.drm_format);
    EGLint eglWidth = width;
    if (surface.fourcc == VA_FOURCC_Y210 || surface.fourcc == VA_FOURCC_Y212 ||
        surface.fourcc == VA_FOURCC_Y216)
    {
      eglFourcc = DRM_FORMAT_ABGR16161616;
      eglWidth = (width + 1) >> 1;
    }

    CEGLAttributes<8> attribs; // 6 static + 2 modifiers
    attribs.Add({{EGL_LINUX_DRM_FOURCC_EXT, eglFourcc},
                 {EGL_WIDTH, eglWidth},
                 {EGL_HEIGHT, height},
                 {EGL_DMA_BUF_PLANE0_FD_EXT, object.fd},
                 {EGL_DMA_BUF_PLANE0_OFFSET_EXT, static_cast<EGLint>(layer.offset[plane])},
                 {EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(layer.pitch[plane])}});

    if (m_hasPlaneModifiers)
    {
      attribs.Add({{EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, static_cast<EGLint>(object.drm_format_modifier)},
        {EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, static_cast<EGLint>(object.drm_format_modifier >> 32)}});
    }

    texture->eglImage = m_interop.eglCreateImageKHR(m_interop.eglDisplay,
      EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr,
      attribs.Get());
    if (!texture->eglImage)
    {
      CEGLUtils::Log(LOGERROR, "Failed to import VA DRM surface into EGL image");
      return failMap();
    }

    glGenTextures(1, &texture->glTexture);
    glBindTexture(m_interop.textureTarget, texture->glTexture);
    m_interop.glEGLImageTargetTexture2DOES(m_interop.textureTarget, texture->eglImage);
    glBindTexture(m_interop.textureTarget, 0);
  }

  return true;
}

void CVaapi2Texture::Unmap()
{
  if (!m_vaapiPic)
    return;

  for (auto texture : {&m_y, &m_vu})
  {
    if (texture->eglImage != EGL_NO_IMAGE_KHR)
    {
      m_interop.eglDestroyImageKHR(m_interop.eglDisplay, texture->eglImage);
      texture->eglImage = EGL_NO_IMAGE_KHR;
      glDeleteTextures(1, &texture->glTexture);
    }
  }

  for (auto& fd : m_drmFDs)
  {
    fd.reset();
  }

  m_vaapiPic->Release();
  m_vaapiPic = nullptr;
}

GLuint CVaapi2Texture::GetTextureY()
{
  return m_y.glTexture;
}

GLuint CVaapi2Texture::GetTextureVU()
{
  return m_vu.glTexture;
}

CSizeInt CVaapi2Texture::GetTextureSize()
{
  return m_textureSize;
}

bool CVaapi2Texture::TestEsh(VADisplay vaDpy, EGLDisplay eglDisplay, std::uint32_t rtFormat, std::int32_t pixelFormat)
{
  int width = 1920;
  int height = 1080;

  PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
  PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
  if (!eglCreateImageKHR || !eglDestroyImageKHR)
  {
    return false;
  }

  // create surfaces
  VASurfaceID surface;
  VAStatus status;

  VASurfaceAttrib attribs = {};
  attribs.flags = VA_SURFACE_ATTRIB_SETTABLE;
  attribs.type = VASurfaceAttribPixelFormat;
  attribs.value.type = VAGenericValueTypeInteger;
  attribs.value.value.i = pixelFormat;

  if (vaCreateSurfaces(vaDpy, rtFormat,
        width, height,
        &surface, 1, &attribs, 1) != VA_STATUS_SUCCESS)
  {
    return false;
  }

  // check interop
  VADRMPRIMESurfaceDescriptor drmPrimeSurface;
  status = vaExportSurfaceHandle(vaDpy, surface,
    VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
    VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS,
    &drmPrimeSurface);

  bool result = false;

  if (status == VA_STATUS_SUCCESS)
  {
    auto const& layer = drmPrimeSurface.layers[0];
    if (layer.object_index[0] >= drmPrimeSurface.num_objects)
    {
      CLog::Log(LOGERROR, "CVaapi2Texture::TestEsh: object_index {} >= num_objects {}",
                layer.object_index[0], drmPrimeSurface.num_objects);
      return false;
    }
    auto const& object = drmPrimeSurface.objects[layer.object_index[0]];
    EGLint attribs[] = {
      EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(drmPrimeSurface.layers[0].drm_format),
      EGL_WIDTH, width,
      EGL_HEIGHT, height,
      EGL_DMA_BUF_PLANE0_FD_EXT, static_cast<EGLint>(object.fd),
      EGL_DMA_BUF_PLANE0_OFFSET_EXT, static_cast<EGLint>(layer.offset[0]),
      EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(layer.pitch[0]),
      EGL_NONE};

    EGLImageKHR eglImage = eglCreateImageKHR(eglDisplay,
      EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr,
      attribs);
    if (eglImage)
    {
      eglDestroyImageKHR(eglDisplay, eglImage);
      result = true;
    }

    for (uint32_t object = 0; object < drmPrimeSurface.num_objects; object++)
    {
      close(drmPrimeSurface.objects[object].fd);
    }
  }

  vaDestroySurfaces(vaDpy, &surface, 1);

  return result;
}

void CVaapi2Texture::TestInterop(VADisplay vaDpy, EGLDisplay eglDisplay, bool& general, bool& deepColor)
{
  general = false;
  deepColor = false;

  general = TestInteropGeneral(vaDpy, eglDisplay);
  if (general)
  {
    deepColor = TestEsh(vaDpy, eglDisplay, VA_RT_FORMAT_YUV420_10BPP, VA_FOURCC_P010);
  }
}

bool CVaapi2Texture::TestInteropGeneral(VADisplay vaDpy, EGLDisplay eglDisplay)
{
  return TestEsh(vaDpy, eglDisplay, VA_RT_FORMAT_YUV420, VA_FOURCC_NV12);
}

void CVaapi2Texture::TestInteropFormats(VADisplay vaDpy, EGLDisplay eglDisplay, CCapabilities& caps)
{
  // NV12 is the importability gate. If EGL cannot import the baseline 8-bit
  // 4:2:0 surface, no other VAAPI fourcc will import either; skip the rest.
  if (!TestEsh(vaDpy, eglDisplay, VA_RT_FORMAT_YUV420, VA_FOURCC_NV12))
    return;

  // Iterate the central format table; any fourcc whose TestEsh round-trip
  // succeeds is added to caps. Future fourccs are added by extending the
  // table - no probe-side changes needed.
  for (const auto& fmt : kVaFormatTable)
  {
    if (TestEsh(vaDpy, eglDisplay, fmt.vaRtFormat, fmt.vaFourcc))
      caps.Add(fmt.pixFmt);
  }
}
