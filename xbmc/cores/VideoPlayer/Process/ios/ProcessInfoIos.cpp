/*
 *      Copyright (C) 2005-2016 Team XBMC
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

