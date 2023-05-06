/*
*  Copyright (C) 2023 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#include "OSScreenSaverWebOS.h"

#include "CompileInfo.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/log.h"

namespace
{
constexpr const char* LUNA_REGISTER_SCREENSAVER =
    "luna://com.webos.service.tvpower/power/registerScreenSaverRequest";
constexpr const char* LUNA_RESPONSE_SCREENSAVER =
    "luna://com.webos.service.tvpower/power/responseScreenSaverRequest";
} // namespace

namespace KODI::WINDOWING::WAYLAND
{

COSScreenSaverWebOS::~COSScreenSaverWebOS()
{
  if (m_requestContext)
  {
    // Luna helper functions return 0 on success
    if (HUnregisterServiceCallback(m_requestContext.get()))
      CLog::LogF(LOGWARNING, "COSScreenSaverWebOS: Luna request unregister failed");
    m_requestContext = nullptr;
  }
}

void COSScreenSaverWebOS::Inhibit()
{
  CVariant request;
  request["subscribe"] = true;
  request["clientName"] = CCompileInfo::GetPackage();
  std::string payload;
  CJSONVariantWriter::Write(request, payload, true);

  m_requestContext = std::make_unique<HContext>();
  m_requestContext->pub = true;
  m_requestContext->multiple = true;
  m_requestContext->callback = &OnScreenSaverAboutToStart;
  if (HLunaServiceCall(LUNA_REGISTER_SCREENSAVER, payload.c_str(), m_requestContext.get()))
  {
    CLog::LogF(LOGWARNING, "COSScreenSaverWebOS: Luna request call failed");
    if (HUnregisterServiceCallback(m_requestContext.get()))
      CLog::LogF(LOGWARNING, "COSScreenSaverWebOS: Luna request unregister failed");
    m_requestContext = nullptr;
  }
}

void COSScreenSaverWebOS::Uninhibit()
{
  if (m_requestContext)
  {
    if (HUnregisterServiceCallback(m_requestContext.get()))
      CLog::LogF(LOGWARNING, "COSScreenSaverWebOS: Luna request unregister failed");
    m_requestContext = nullptr;
  }
}

bool COSScreenSaverWebOS::OnScreenSaverAboutToStart(LSHandle* sh, LSMessage* reply, void* ctx)
{
  CVariant request;
  const char* msg = HLunaServiceMessage(reply);
  CJSONVariantParser::Parse(msg, request);

  CLog::LogF(LOGDEBUG, "COSScreenSaverWebOS: Responded {}", msg);

  if (request["state"] != "Active")
    return true;

  CVariant response;
  response["clientName"] = CCompileInfo::GetPackage();
  response["ack"] = false;
  response["timestamp"] = request["timestamp"];
  std::string payload;
  CJSONVariantWriter::Write(response, payload, true);

  HContext response_ctx;
  response_ctx.multiple = false;
  response_ctx.pub = true;
  response_ctx.callback = nullptr;
  if (HLunaServiceCall(LUNA_RESPONSE_SCREENSAVER, payload.c_str(), &response_ctx))
  {
    CLog::LogF(LOGWARNING, "COSScreenSaverWebOS: Luna response call failed");
    return false;
  }

  return true;
}

} // namespace KODI::WINDOWING::WAYLAND
