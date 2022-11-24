/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPProcessInfoWayland.h"

using namespace KODI;
using namespace RETRO;

CRPProcessInfoWayland::CRPProcessInfoWayland() : CRPProcessInfo("Wayland")
{
}

std::unique_ptr<CRPProcessInfo> CRPProcessInfoWayland::Create()
{
  return std::make_unique<CRPProcessInfoWayland>();
}

void CRPProcessInfoWayland::Register()
{
  CRPProcessInfo::RegisterProcessControl(CRPProcessInfoWayland::Create);
}
