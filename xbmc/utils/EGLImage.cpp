/*
 *      Copyright (C) 2017 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "EGLImage.h"
#include "EGLUtils.h"
#include "log.h"

/* --- CEGLImage -------------------------------------------*/

namespace
{
  const EGLint eglDmabufPlaneFdAttr[CEGLImage::MAX_NUM_PLANES] =
  {
    EGL_DMA_BUF_PLANE0_FD_EXT,
    EGL_DMA_BUF_PLANE1_FD_EXT,
    EGL_DMA_BUF_PLANE2_FD_EXT,
  };

  const EGLint eglDmabufPlaneOffsetAttr[CEGLImage::MAX_NUM_PLANES] =
  {
    EGL_DMA_BUF_PLANE0_OFFSET_EXT,
    EGL_DMA_BUF_PLANE1_OFFSET_EXT,
    EGL_DMA_BUF_PLANE2_OFFSET_EXT,
  };

  const EGLint eglDmabufPlanePitchAttr[CEGLImage::MAX_NUM_PLANES] =
  {
    EGL_DMA_BUF_PLANE0_PITCH_EXT,
    EGL_DMA_BUF_PLANE1_PITCH_EXT,
    EGL_DMA_BUF_PLANE2_PITCH_EXT,
  };

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
  const EGLint eglDmabufPlaneModifierLoAttr[CEGLImage::MAX_NUM_PLANES] =
  {
    EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
    EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
    EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT,
  };

  const EGLint eglDmabufPlaneModifierHiAttr[CEGLImage::MAX_NUM_PLANES] =
  {
    EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT,
    EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT,
    EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT,
  };
#endif
} // namespace

CEGLImage::CEGLImage(EGLDisplay display) :
  m_display(display)
{
  m_eglCreateImageKHR = CEGLUtils::GetRequiredProcAddress<PFNEGLCREATEIMAGEKHRPROC>("eglCreateImageKHR");
  m_eglDestroyImageKHR = CEGLUtils::GetRequiredProcAddress<PFNEGLDESTROYIMAGEKHRPROC>("eglDestroyImageKHR");
  m_glEGLImageTargetTexture2DOES = CEGLUtils::GetRequiredProcAddress<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>("glEGLImageTargetTexture2DOES");
}

bool CEGLImage::CreateImage(EglAttrs imageAttrs)
{
  CEGLAttributes<22> attribs;
  attribs.Add({{EGL_WIDTH, imageAttrs.width},
               {EGL_HEIGHT, imageAttrs.height},
               {EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(imageAttrs.format)}});

  /* this should be configurable at a later point */
  attribs.Add({{EGL_YUV_COLOR_SPACE_HINT_EXT, EGL_ITU_REC709_EXT},
               {EGL_SAMPLE_RANGE_HINT_EXT, EGL_YUV_NARROW_RANGE_EXT},
               {EGL_YUV_CHROMA_VERTICAL_SITING_HINT_EXT, EGL_YUV_CHROMA_SITING_0_EXT},
               {EGL_YUV_CHROMA_HORIZONTAL_SITING_HINT_EXT, EGL_YUV_CHROMA_SITING_0_EXT}});

  for (int i = 0; i < MAX_NUM_PLANES; i++)
  {
    if (imageAttrs.planes[i].fd != 0)
    {
      attribs.Add({{eglDmabufPlaneFdAttr[i], imageAttrs.planes[i].fd},
                   {eglDmabufPlaneOffsetAttr[i], imageAttrs.planes[i].offset},
                   {eglDmabufPlanePitchAttr[i], imageAttrs.planes[i].pitch}});

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
      if (imageAttrs.planes[i].modifier != DRM_FORMAT_MOD_INVALID)
        attribs.Add({{eglDmabufPlaneModifierLoAttr[i], static_cast<EGLint>(imageAttrs.planes[i].modifier & 0xFFFFFFFF)},
                     {eglDmabufPlaneModifierHiAttr[i], static_cast<EGLint>(imageAttrs.planes[i].modifier >> 32)}});
#endif
    }
  }

  m_image = m_eglCreateImageKHR(m_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attribs.Get());

  if(!m_image)
  {
    CLog::Log(LOGERROR, "CEGLImage::%s - failed to import buffer into EGL image: %d", __FUNCTION__, eglGetError());
    return false;
  }

  return true;
}

void CEGLImage::UploadImage(GLenum textureTarget)
{
  m_glEGLImageTargetTexture2DOES(textureTarget, m_image);
}

void CEGLImage::DestroyImage()
{
  m_eglDestroyImageKHR(m_display, m_image);
}
