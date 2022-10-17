/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HTTPWebinterfaceAddonsHandler.h"

#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "network/WebServer.h"

#define ADDON_HEADER      "<html><head><title>Add-on List</title></head><body>\n<h1>Available web interfaces:</h1>\n<ul>\n"

bool CHTTPWebinterfaceAddonsHandler::CanHandleRequest(const HTTPRequest &request) const
{
  return (request.pathUrl.compare("/addons") == 0 || request.pathUrl.compare("/addons/") == 0);
}

MHD_RESULT CHTTPWebinterfaceAddonsHandler::HandleRequest()
{
  m_responseData = ADDON_HEADER;
  ADDON::VECADDONS addons;
  if (!CServiceBroker::GetAddonMgr().GetAddons(addons, ADDON::AddonType::WEB_INTERFACE) ||
      addons.empty())
  {
    m_response.type = HTTPError;
    m_response.status = MHD_HTTP_INTERNAL_SERVER_ERROR;

    return MHD_YES;
  }

  for (const auto& addon : addons)
    m_responseData += "<li><a href=/addons/" + addon->ID() + "/>" + addon->Name() + "</a></li>\n";

  m_responseData += "</ul>\n</body></html>";

  m_responseRange.SetData(m_responseData.c_str(), m_responseData.size());

  m_response.type = HTTPMemoryDownloadNoFreeCopy;
  m_response.status = MHD_HTTP_OK;
  m_response.contentType = "text/html";
  m_response.totalLength = m_responseData.size();

  return MHD_YES;
}

HttpResponseRanges CHTTPWebinterfaceAddonsHandler::GetResponseData() const
{
  HttpResponseRanges ranges;
  ranges.push_back(m_responseRange);

  return ranges;
}


