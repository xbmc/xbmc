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

#include <algorithm>
#include <array>
#include <charconv>
#include <string_view>

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

constexpr const char* LUNA_ARC_STATUS = "luna://com.webos.service.arccontroller/getARCState";
constexpr const char* LUNA_GET_SYSTEM_SETTINGS =
    "luna://com.webos.settingsservice/getSystemSettings";

constexpr const char* SETTINGS = "settings";
constexpr const char* SOUND_OUTPUT_DIGITAL = "soundOutputDigital";
constexpr const char* PASS_THROUGH = "passThrough";

constexpr const char* ARC_ACTIVATION = "arcActivation";
constexpr const char* EARC_CAPABILITY = "earcCapability";
constexpr const char* ADVANCED_EARC_CAPABILITY = "advancedEarcCapability";
constexpr const char* AUDIO_CODING_TYPE = "audioCodingType";
constexpr const char* HEX_SAD = "hexSAD";
constexpr const char* PCM = "PCM";
constexpr std::array<std::string_view, 5> EARC_FORMATS{"TrueHD", "DTSHD_MA", "DTS", "DD", "DD+"};
constexpr std::array<unsigned int, 7> SAD_SAMPLE_RATES{32000, 44100,  48000, 88200,
                                                       96000, 176400, 192000};
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
  m_requestContextARC->pub = true;
  m_requestContextARC->multiple = true;
  m_requestContextARC->callback = [](LSHandle* sh, LSMessage* msg, void* ctx) -> bool
  {
    auto* self = static_cast<WebOSTVPlatformConfig*>(ctx);
    std::string message = HLunaServiceMessage(msg);
    CLog::LogF(LOGDEBUG, "ARC controller: {}", message);

    CVariant parsed;
    if (!CJSONVariantParser::Parse(message, parsed))
    {
      CLog::LogF(LOGERROR, "Failed to parse ARC controller JSON");
      return false;
    }

    const std::string arcActivation = parsed[ARC_ACTIVATION].asString();
    m_eARCActive = arcActivation == EARC;

    const CVariant& sads = parsed[ADVANCED_EARC_CAPABILITY];
    unsigned int formats = 0;
    std::uint64_t pcm = 0;
    for (auto sad = sads.begin_array(); m_eARCActive && sad != sads.end_array(); ++sad)
    {
      const std::string type = (*sad)[AUDIO_CODING_TYPE].asString();
      if (const auto format = std::ranges::find(EARC_FORMATS, std::string_view(type));
          format != EARC_FORMATS.end())
        formats |= 1u << (format - EARC_FORMATS.begin());

      const std::string hex = (*sad)[HEX_SAD].asString();
      unsigned int value = 0;
      if (type == PCM &&
          std::from_chars(hex.data(), hex.data() + hex.size(), value, 16).ec == std::errc())
        for (unsigned int channel = 0; channel <= ((value >> 16) & 7); ++channel)
          pcm |= static_cast<std::uint64_t>((value >> 8) & 0x7f) << (channel * 8);
    }
    m_eARCFormats = formats;
    m_eARCPCM = pcm;

    if (arcActivation == EARC)
    {
      const CVariant& caps = parsed[EARC_CAPABILITY];
      self->m_eAC3Supported =
          caps.isArray() && std::any_of(caps.begin_array(), caps.end_array(),
                                        [](const CVariant& v) { return v.asString() == DDPLUS; });
    }

    return true;
  };

  if (HLunaServiceCall(LUNA_ARC_STATUS, R"({"subscribe":true})", m_requestContextARC.get()))
  {
    CLog::LogF(LOGWARNING, "Luna ARC controller request call failed");
  }
}

bool WebOSTVPlatformConfig::SupportsEAC3()
{
  return m_eAC3Supported;
}

void WebOSTVPlatformConfig::LoadSoundOutputStatus()
{
  m_requestContextSoundOutput->pub = true;
  m_requestContextSoundOutput->multiple = true;
  m_requestContextSoundOutput->callback = [](LSHandle* sh, LSMessage* msg, void* ctx) -> bool
  {
    CVariant parsed;
    if (CJSONVariantParser::Parse(HLunaServiceMessage(msg), parsed) &&
        parsed[SETTINGS].isMember(SOUND_OUTPUT_DIGITAL))
      m_digitalPassThrough = parsed[SETTINGS][SOUND_OUTPUT_DIGITAL].asString() == PASS_THROUGH;
    return true;
  };

  if (HLunaServiceCall(LUNA_GET_SYSTEM_SETTINGS,
                       R"({"category":"sound","keys":["soundOutputDigital"],"subscribe":true})",
                       m_requestContextSoundOutput.get()))
    CLog::LogF(LOGWARNING, "Luna sound output settings request call failed");
}

bool WebOSTVPlatformConfig::SupportsEARCPassthrough()
{
  return m_eARCActive && m_digitalPassThrough;
}

bool WebOSTVPlatformConfig::SupportsEARCFormat(std::string_view type)
{
  const auto format = std::ranges::find(EARC_FORMATS, type);
  return format != EARC_FORMATS.end() && m_eARCFormats & (1u << (format - EARC_FORMATS.begin()));
}

unsigned int WebOSTVPlatformConfig::GetEARCPCMRate(unsigned int channels, unsigned int sampleRate)
{
  if (channels < 1 || channels > 8 || sampleRate == 0)
    return 0;

  const std::uint64_t rates = m_eARCPCM >> ((channels - 1) * 8);
  unsigned int multiple = 0;
  unsigned int above = 0;
  unsigned int divisor = 0;
  unsigned int below = 0;
  for (std::size_t i = 0; i < SAD_SAMPLE_RATES.size(); ++i)
  {
    const unsigned int rate = SAD_SAMPLE_RATES[i];
    if (!(rates & (1u << i)))
      continue;

    if (rate < sampleRate)
    {
      below = rate;
      if (sampleRate % rate == 0)
        divisor = rate;
    }
    else
    {
      if (above == 0)
        above = rate;
      if (multiple == 0 && rate % sampleRate == 0)
        multiple = rate;
    }
  }
  return multiple ? multiple : above ? above : divisor ? divisor : below;
}
