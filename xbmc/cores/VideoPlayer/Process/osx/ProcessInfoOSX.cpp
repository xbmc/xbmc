/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoOSX.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "threads/SingleLock.h"

using namespace VIDEOPLAYER;

CProcessInfo* CProcessInfoOSX::Create()
{
  return new CProcessInfoOSX();
}

void CProcessInfoOSX::Register()
{
  CProcessInfo::RegisterProcessControl("osx", CProcessInfoOSX::Create);
}

void CProcessInfoOSX::SetSwDeinterlacingMethods()
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

std::vector<AVPixelFormat> CProcessInfoOSX::GetRenderFormats()
{
  std::vector<AVPixelFormat> formats;
  formats.push_back(AV_PIX_FMT_YUV420P);
  formats.push_back(AV_PIX_FMT_YUV420P10);
  formats.push_back(AV_PIX_FMT_YUV420P16);
  formats.push_back(AV_PIX_FMT_NV12);
  formats.push_back(AV_PIX_FMT_YUYV422);
  formats.push_back(AV_PIX_FMT_UYVY422);

  return formats;
}

