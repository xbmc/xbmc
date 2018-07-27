/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPProcessInfoPi.h"

using namespace KODI;
using namespace RETRO;

CRPProcessInfoPi::CRPProcessInfoPi() :
  CRPProcessInfo("RPi")
{
}

CRPProcessInfo* CRPProcessInfoPi::Create()
{
  return new CRPProcessInfoPi();
}

void CRPProcessInfoPi::Register()
{
  CRPProcessInfo::RegisterProcessControl(CRPProcessInfoPi::Create);
}
