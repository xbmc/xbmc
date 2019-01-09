/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoAndroid.h"

using namespace VIDEOPLAYER;

CProcessInfo* CProcessInfoAndroid::Create()
{
  return new CProcessInfoAndroid();
}

CProcessInfoAndroid::CProcessInfoAndroid()
{
}

void CProcessInfoAndroid::Register()
{
  CProcessInfo::RegisterProcessControl("android", CProcessInfoAndroid::Create);
}

EINTERLACEMETHOD CProcessInfoAndroid::GetFallbackDeintMethod()
{
  return EINTERLACEMETHOD::VS_INTERLACEMETHOD_DEINTERLACE_HALF;
}
