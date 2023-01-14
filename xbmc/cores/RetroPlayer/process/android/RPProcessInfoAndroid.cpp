/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPProcessInfoAndroid.h"

using namespace KODI;
using namespace RETRO;

CRPProcessInfoAndroid::CRPProcessInfoAndroid() : CRPProcessInfo("Android")
{
}

std::unique_ptr<CRPProcessInfo> CRPProcessInfoAndroid::Create()
{
  return std::make_unique<CRPProcessInfoAndroid>();
}

void CRPProcessInfoAndroid::Register()
{
  CRPProcessInfo::RegisterProcessControl(CRPProcessInfoAndroid::Create);
}
