/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPProcessInfoGbm.h"

using namespace KODI;
using namespace RETRO;

CRPProcessInfoGbm::CRPProcessInfoGbm() : CRPProcessInfo("GBM")
{
}

CRPProcessInfo* CRPProcessInfoGbm::Create()
{
  return new CRPProcessInfoGbm();
}

void CRPProcessInfoGbm::Register()
{
  CRPProcessInfo::RegisterProcessControl(CRPProcessInfoGbm::Create);
}
