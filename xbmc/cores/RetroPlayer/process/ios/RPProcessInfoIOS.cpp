/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPProcessInfoIOS.h"

using namespace KODI;
using namespace RETRO;

CRPProcessInfoIOS::CRPProcessInfoIOS() : CRPProcessInfo("iOS")
{
}

std::unique_ptr<CRPProcessInfo> CRPProcessInfoIOS::Create()
{
  return std::make_unique<CRPProcessInfoIOS>();
}

void CRPProcessInfoIOS::Register()
{
  CRPProcessInfo::RegisterProcessControl(CRPProcessInfoIOS::Create);
}
