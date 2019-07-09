/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoIOS.h"

#include "threads/SingleLock.h"

using namespace VIDEOPLAYER;

CProcessInfo* CProcessInfoIOS::Create()
{
  return new CProcessInfoIOS();
}

void CProcessInfoIOS::Register()
{
  CProcessInfo::RegisterProcessControl("ios", CProcessInfoIOS::Create);
}

void CProcessInfoIOS::SetSwDeinterlacingMethods()
{
  // first populate with the defaults from base implementation
  CProcessInfo::SetSwDeinterlacingMethods();

  std::list<EINTERLACEMETHOD> methods;
  {
    // get the current methods
    CSingleLock lock(m_videoCodecSection);
    methods = m_deintMethods;
  }
  // add bob deinterlacer for ios
  methods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_RENDER_BOB);

  // update with the new methods list
  UpdateDeinterlacingMethods(methods);
}

