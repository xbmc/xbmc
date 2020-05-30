/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPProcessInfoOSX.h"

using namespace KODI;
using namespace RETRO;

CRPProcessInfoOSX::CRPProcessInfoOSX() : CRPProcessInfo("macOS")
{
}

CRPProcessInfo* CRPProcessInfoOSX::Create()
{
  return new CRPProcessInfoOSX();
}

void CRPProcessInfoOSX::Register()
{
  CRPProcessInfo::RegisterProcessControl(CRPProcessInfoOSX::Create);
}
