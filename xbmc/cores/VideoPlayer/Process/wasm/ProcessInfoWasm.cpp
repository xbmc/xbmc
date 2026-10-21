/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoWasm.h"

CProcessInfo* CProcessInfoWasm::Create()
{
  return new CProcessInfoWasm();
}

void CProcessInfoWasm::Register()
{
  CProcessInfo::RegisterProcessControl("wasm", CProcessInfoWasm::Create);
}

EINTERLACEMETHOD CProcessInfoWasm::GetFallbackDeintMethod()
{
  return EINTERLACEMETHOD::VS_INTERLACEMETHOD_DEINTERLACE_HALF;
}
