/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "HTTPWebinterfaceAddonsHandler.h"
#include "addons/AddonManager.h"
#include "network/WebServer.h"

#define ADDON_HEADER      "<html><head><title>Add-on List</title></head><body>\n<h1>Available web interfaces:</h1>\n<ul>\n"

bool CHTTPWebinterfaceAddonsHandler::CanHandleRequest(const HTTPRequest &request)
{
  return (request.pathUrl.compare("/addons") == 0 || request.pathUrl.compare("/addons/") == 0);
}

int CHTTPWebinterfaceAddonsHandler::HandleRequest()
{
  m_responseData = ADDON_HEADER;
  ADDON::VECADDONS addons;
  if (!ADDON::CAddonMgr::Get().GetAddons(ADDON::ADDON_WEB_INTERFACE, addons) || addons.empty())
  {
    m_response.type = HTTPError;
    m_response.status = MHD_HTTP_INTERNAL_SERVER_ERROR;

    return MHD_YES;
  }

  for (ADDON::IVECADDONS addon = addons.begin(); addon != addons.end(); ++addon)
    m_responseData += "<li><a href=/addons/" + (*addon)->ID() + "/>" + (*addon)->Name() + "</a></li>\n";

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


