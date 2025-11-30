/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebOSTVPlatformConfig.h"

#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <webos-helpers/libhelpers.h>

namespace
{
constexpr const char* LUNA_GET_CONFIG = "luna://com.webos.service.config/getConfigs";

constexpr const char* CONFIGS = "configs";
constexpr const char* CONFIG_NAMES = "configNames";
const auto QUERY_CONFIG = std::vector<std::string>{"tv.model.*", "tv.nyx.*"};

constexpr const char* EDID_TYPE = "tv.model.edidType";
constexpr const char* SUPPORT_HDR = "tv.model.supportHDR";

constexpr const char* PLATFORM_CODE = "tv.nyx.platformCode";

constexpr const char* DTS = "dts";

constexpr const char* EARC_ACTIVATION = "arcActivation";
constexpr const char* EARC_CAPABILITY = "earcCapability";
constexpr const char* SUPPORT_EARCDDPlus = "eARCDDPlus";
constexpr const char* EARC = "eARC";
constexpr const char* DDPLUS = "DD+";

CVariant ms_config;
} // namespace

void WebOSTVPlatformConfig::Load()
{
  HContext requestContext;
  requestContext.pub = true;
  requestContext.multiple = false;
  requestContext.callback = [](LSHandle* sh, LSMessage* msg, void* ctx)
  {
    std::string message = HLunaServiceMessage(msg);
    CLog::LogF(LOGDEBUG, "TV config: {}", message);
    if (!CJSONVariantParser::Parse(message, ms_config))
    {
      CLog::LogF(LOGERROR, "Failed to parse config JSON from Luna service");
      return false;
    }
    ms_config = CVariant(ms_config[CONFIGS]);
    return false;
  };

  CVariant request;
  request[CONFIG_NAMES] = QUERY_CONFIG;
  std::string payload;
  CJSONVariantWriter::Write(request, payload, true);
  if (HLunaServiceCall(LUNA_GET_CONFIG, payload.c_str(), &requestContext))
  {
    CLog::LogF(LOGWARNING, "Luna get config request call failed");
  }
}

int WebOSTVPlatformConfig::GetWebOSVersion()
{
  return ms_config[PLATFORM_CODE].asInteger();
}

bool WebOSTVPlatformConfig::SupportsDTS()
{
  return ms_config[EDID_TYPE].asString().find(DTS) != std::string::npos;
}

bool WebOSTVPlatformConfig::SupportsHDR()
{
  return ms_config[SUPPORT_HDR].asBoolean();
}

void WebOSTVPlatformConfig::LoadARCStatus()
{
  HContext requestContext;
  requestContext.pub = true;
  requestContext.multiple = false;
  requestContext.callback = [](LSHandle* sh, LSMessage* msg, void* ctx) -> bool
  {
    std::string message = HLunaServiceMessage(msg);
    CLog::LogF(LOGDEBUG, "ARC controller: {}", message);

    CVariant parsed;
    if (!CJSONVariantParser::Parse(message, parsed))
    {
      CLog::LogF(LOGERROR, "Failed to parse ARC controller JSON");
      return false;
    }

    const std::string arcActivation = parsed[EARC_ACTIVATION].asString();
    bool ddplusSupported = false;

    if (arcActivation == EARC)
    {
      const CVariant& caps = parsed[EARC_CAPABILITY];
      if (caps.isArray())
      {
        for (unsigned int i = 0; i < caps.size(); ++i)
        {
          if (caps[i].asString() == DDPLUS)
          {
            ddplusSupported = true;
            break;
          }
        }
      }
    }

    ms_config[SUPPORT_EARCDDPlus] = ddplusSupported;

    return true;
  };

  std::string payload = "{}";
  constexpr const char* LUNA_ARC_STATUS = "luna://com.webos.service.arccontroller/getARCState";

  if (HLunaServiceCall(LUNA_ARC_STATUS, payload.c_str(), &requestContext))
  {
    CLog::LogF(LOGWARNING, "Luna ARC controller request call failed");
  }
}

bool WebOSTVPlatformConfig::SupportsEARCDDPlus()
{
  LoadARCStatus();
  return ms_config[SUPPORT_EARCDDPlus].asBoolean();
}
