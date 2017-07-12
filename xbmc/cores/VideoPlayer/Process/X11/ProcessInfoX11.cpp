/*
 *      Copyright (C) 2005-2016 Team XBMC
 *      http://xbmc.org
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

#include "ProcessInfoX11.h"
#include "threads/SingleLock.h"

using namespace VIDEOPLAYER;

CProcessInfo* CProcessInfoX11::Create()
{
  return new CProcessInfoX11();
}

void CProcessInfoX11::Register()
{
  CProcessInfo::RegisterProcessControl("X11", CProcessInfoX11::Create);
}

void CProcessInfoX11::SetSwDeinterlacingMethods()
{
  // first populate with the defaults from base implementation
  CProcessInfo::SetSwDeinterlacingMethods();

  std::list<EINTERLACEMETHOD> methods;
  {
    // get the current methods
    CSingleLock lock(m_videoCodecSection);
    methods = m_deintMethods;
  }
  // add bob and blend deinterlacer for osx
  methods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_RENDER_BOB);
  methods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_RENDER_BLEND);

  // update with the new methods list
  UpdateDeinterlacingMethods(methods);
}

std::vector<AVPixelFormat> CProcessInfoX11::GetRenderFormats()
{
  return
  {
    AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_YUV420P9,
    AV_PIX_FMT_YUV420P10,
    AV_PIX_FMT_YUV420P12,
    AV_PIX_FMT_YUV420P14,
    AV_PIX_FMT_YUV420P16,
    AV_PIX_FMT_NV12,
    AV_PIX_FMT_YUYV422,
    AV_PIX_FMT_UYVY422
  };
}
