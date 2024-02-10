/*
*  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LunaPowerManagement.h"

#include "utils/JSONVariantWriter.h"
#include "utils/Variant.h"

#include <webos-helpers/libhelpers.h>

namespace
{
constexpr const char* LUNA_POWEROFF = "luna://com.webos.service.tvpower/power/powerOff";
constexpr const char* LUNA_REBOOT = "luna://com.webos.service.tvpower/power/reboot";
} // namespace

bool CLunaPowerManagement::Powerdown()
{
  HContext context;
  context.multiple = false;
  context.pub = true;
  context.callback = nullptr;

  CVariant shutdown;
  shutdown["reason"] = "remoteKey";
  std::string json;
  CJSONVariantWriter::Write(shutdown, json, true);

  return HLunaServiceCall(LUNA_POWEROFF, json.c_str(), &context) == 0;
}

bool CLunaPowerManagement::Reboot()
{
  HContext context;
  context.multiple = false;
  context.pub = true;
  context.callback = nullptr;

  CVariant reboot;
  reboot["reason"] = "remoteKey";
  std::string json;
  CJSONVariantWriter::Write(reboot, json, true);

  return HLunaServiceCall(LUNA_REBOOT, json.c_str(), &context) == 0;
}

IPowerSyscall* CLunaPowerManagement::CreateInstance()
{
  return new CLunaPowerManagement();
}

void CLunaPowerManagement::Register()
{
  RegisterPowerSyscall(CreateInstance);
}
