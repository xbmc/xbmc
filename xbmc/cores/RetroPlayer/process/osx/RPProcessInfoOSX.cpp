/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPProcessInfoOSX.h"

#include <dlfcn.h>

using namespace KODI;
using namespace RETRO;

CRPProcessInfoOSX::CRPProcessInfoOSX() : CRPProcessInfo("macOS")
{
}

std::unique_ptr<CRPProcessInfo> CRPProcessInfoOSX::Create()
{
  return std::make_unique<CRPProcessInfoOSX>();
}

void CRPProcessInfoOSX::Register()
{
  CRPProcessInfo::RegisterProcessControl(CRPProcessInfoOSX::Create);
}

HwProcedureAddress CRPProcessInfoOSX::GetHwProcedureAddress(const char* symbol)
{
  return symbol ? reinterpret_cast<HwProcedureAddress>(dlsym(RTLD_DEFAULT, symbol)) : nullptr;
}
