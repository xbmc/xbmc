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
#include <unistd.h>
#include <va/va_drmcommon.h>

using namespace VAAPI;

void CVaapi2Texture::Init(InteropInfo& interop)
{
  m_interop = interop;
  m_hasPlaneModifiers = CEGLUtils::HasExtension(m_interop.eglDisplay, "EGL_EXT_image_dma_buf_import_modifiers");
}

bool CVaapi2Texture::Import(CVaapiRenderPicture* pic)
{
  if (m_imported)
    return true;

  auto failImport = [this]()
  {
    // also destroys images/textures a partly failed import created
    Reset();
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
    CLog::Log(LOGWARNING, "CVaapi2Texture::Import: vaExportSurfaceHandle failed - Error: {} ({})",
              vaErrorStr(status), status);
    return failImport();
  }

  // Take ownership of the exported fds (RAII closes them on Reset). num_objects
  // is bounded by the libva descriptor's fixed objects[] array == m_drmFDs.size().
  for (uint32_t object = 0; object < surface.num_objects && object < m_drmFDs.size(); object++)
    m_drmFDs[object].attach(surface.objects[object].fd);

  status = vaSyncSurface(pic->vadsp, pic->procPic.videoSurface);
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "CVaapi2Texture::Import: vaSyncSurface - Error: {} ({})",
              vaErrorStr(status), status);
    return failImport();
  }

  m_textureSize.Set(pic->DVDPic.iWidth, pic->DVDPic.iHeight);

  for (uint32_t layerNo = 0; layerNo < surface.num_layers; layerNo++)
  {
    int plane = 0;
    auto const& layer = surface.layers[layerNo];
    if (layer.num_planes != 1)
    {
      CLog::Log(LOGDEBUG,
                "CVaapi2Texture::Import: DRM-exported layer has {} planes - only 1 supported",
                layer.num_planes);
      return failImport();
    }
    // Driver-supplied object_index is untrusted; reject OOB before indexing.
    if (layer.object_index[plane] >= surface.num_objects)
    {
      CLog::Log(LOGERROR,
                "CVaapi2Texture::Import: layer {} plane {} object_index {} >= num_objects {}",
                layerNo, plane, layer.object_index[plane], surface.num_objects);
      return failImport();
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
            failImport();
            throw std::logic_error("Impossible layer number");
        }
        break;
      default:
        CLog::Log(LOGDEBUG,
                  "CVaapi2Texture::Import: DRM-exported surface {} layers - only 1 or 2 supported",
                  surface.num_layers);
        return failImport();
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
      return failImport();
    }

    glGenTextures(1, &texture->glTexture);
    glBindTexture(m_interop.textureTarget, texture->glTexture);
    m_interop.glEGLImageTargetTexture2DOES(m_interop.textureTarget, texture->eglImage);
    glBindTexture(m_interop.textureTarget, 0);
  }

  m_imported = true;
  return true;
}

void CVaapi2Texture::Reset()
{

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

  m_imported = false;
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

namespace
{

// separate-layers export: one plane per layer, keyed like the DRMPRIME path
std::optional<DRMPRIME::DmaBufIdentity> IdentityFromVaDescriptor(
    const VADRMPRIMESurfaceDescriptor& surface)
{
  if (surface.num_objects < 1 || surface.num_objects > AV_DRM_MAX_PLANES ||
      surface.num_layers < 1 || surface.num_layers > AV_DRM_MAX_PLANES)
    return std::nullopt;

  DRMPRIME::DmaBufIdentity identity;
  identity.nbObjects = surface.num_objects;
  for (uint32_t i = 0; i < surface.num_objects; i++)
  {
    if (!DRMPRIME::StatInode(surface.objects[i].fd, identity.inode[i]))
      return std::nullopt;
    identity.modifier[i] = surface.objects[i].drm_format_modifier;
  }

  identity.width = surface.width;
  identity.height = surface.height;
  identity.format = surface.fourcc;
  identity.nbPlanes = surface.num_layers;
  for (uint32_t i = 0; i < surface.num_layers; i++)
  {
    identity.objectIndex[i] = surface.layers[i].object_index[0];
    identity.offset[i] = surface.layers[i].offset[0];
    identity.pitch[i] = surface.layers[i].pitch[0];
  }

  return identity;
}

} // namespace

void CVaapiTexturePool::Init(InteropInfo& interop)
{
  m_interop = interop;
}

CVaapi2Texture* CVaapiTexturePool::Get(CVaapiRenderPicture* pic)
{
  VADRMPRIMESurfaceDescriptor surface;
  VAStatus status = vaExportSurfaceHandle(
      pic->vadsp, pic->procPic.videoSurface, VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
      VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS, &surface);
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGWARNING, "CVaapiTexturePool::{}: vaExportSurfaceHandle failed - Error: {} ({})",
              __FUNCTION__, vaErrorStr(status), status);
    return nullptr;
  }

  // this export only identifies the memory; Import on a miss re-exports its own fds
  const auto identity = IdentityFromVaDescriptor(surface);
  for (uint32_t i = 0; i < surface.num_objects; i++)
    close(surface.objects[i].fd);
  if (!identity)
    return nullptr;

  uint32_t handle = m_cache.Lookup(*identity);
  if (handle)
  {
    // Import syncs only on a miss; a cached texture still needs this frame's decode done
    status = vaSyncSurface(pic->vadsp, pic->procPic.videoSurface);
    if (status != VA_STATUS_SUCCESS)
    {
      CLog::Log(LOGERROR, "CVaapiTexturePool::{}: vaSyncSurface - Error: {} ({})", __FUNCTION__,
                vaErrorStr(status), status);
      return nullptr;
    }
  }
  else
  {
    size_t slot;
    if (!m_free.empty())
    {
      slot = m_free.back();
      m_free.pop_back();
    }
    else
    {
      slot = m_entries.size();
      m_entries.push_back(std::make_unique<CVaapi2Texture>());
      m_entries[slot]->Init(m_interop);
    }

    if (!m_entries[slot]->Import(pic))
    {
      m_free.push_back(slot);
      return nullptr;
    }
    handle = static_cast<uint32_t>(slot) + 1;
    m_cache.Insert(*identity, handle);
  }

  for (uint32_t doomed : m_cache.Reap(handle, 0))
  {
    m_entries[doomed - 1]->Reset();
    m_free.push_back(doomed - 1);
  }

  return m_entries[handle - 1].get();
}

void CVaapiTexturePool::ReleaseAll()
{
  m_cache.TakeAll();
  for (auto& entry : m_entries)
  {
    if (entry)
      entry->Reset();
  }
  m_entries.clear();
  m_free.clear();
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
    if (layer.object_index[0] < drmPrimeSurface.num_objects)
    {
      auto const& object = drmPrimeSurface.objects[layer.object_index[0]];
      EGLint attribs[] = {EGL_LINUX_DRM_FOURCC_EXT,
                          static_cast<EGLint>(drmPrimeSurface.layers[0].drm_format),
                          EGL_WIDTH,
                          width,
                          EGL_HEIGHT,
                          height,
                          EGL_DMA_BUF_PLANE0_FD_EXT,
                          static_cast<EGLint>(object.fd),
                          EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                          static_cast<EGLint>(layer.offset[0]),
                          EGL_DMA_BUF_PLANE0_PITCH_EXT,
                          static_cast<EGLint>(layer.pitch[0]),
                          EGL_NONE};

      EGLImageKHR eglImage =
          eglCreateImageKHR(eglDisplay, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attribs);
      if (eglImage)
      {
        eglDestroyImageKHR(eglDisplay, eglImage);
        result = true;
      }
    }
    else
      CLog::Log(LOGERROR, "CVaapi2Texture::TestEsh: object_index {} >= num_objects {}",
                layer.object_index[0], drmPrimeSurface.num_objects);

    for (uint32_t object = 0; object < drmPrimeSurface.num_objects; object++)
    {
      close(drmPrimeSurface.objects[object].fd);
    }
  }

  vaDestroySurfaces(vaDpy, &surface, 1);

  return result;
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
