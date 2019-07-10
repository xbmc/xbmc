/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoWayland.h"

#include "threads/SingleLock.h"

using namespace VIDEOPLAYER;

CProcessInfo* CProcessInfoWayland::Create()
{
  return new CProcessInfoWayland();
}

void CProcessInfoWayland::Register()
{
  CProcessInfo::RegisterProcessControl("Wayland", CProcessInfoWayland::Create);
}

void CProcessInfoWayland::SetSwDeinterlacingMethods()
{
  // first populate with the defaults from base implementation
  CProcessInfo::SetSwDeinterlacingMethods();

  std::list<EINTERLACEMETHOD> methods;
  {
    // get the current methods
    CSingleLock lock(m_videoCodecSection);
    methods = m_deintMethods;
  }
  // add bob and blend deinterlacer
  methods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_RENDER_BOB);
  methods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_RENDER_BLEND);

  // update with the new methods list
  UpdateDeinterlacingMethods(methods);
}

std::vector<AVPixelFormat> CProcessInfoWayland::GetRenderFormats()
{
  return
  {
    // GL & GLES
    AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_NV12,

#if defined(HAS_GL)
    // Full GL only at the moment
    // TODO YUV420Pxx need runtime-checking for GL_ALPHA16/GL_LUMINANCE16 support
    AV_PIX_FMT_YUV420P9,
    AV_PIX_FMT_YUV420P10,
    AV_PIX_FMT_YUV420P12,
    AV_PIX_FMT_YUV420P14,
    AV_PIX_FMT_YUV420P16,
    AV_PIX_FMT_YUYV422,
    AV_PIX_FMT_UYVY422
#endif
  };
}
