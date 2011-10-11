/*
 *      Copyright (C) 2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "HTTPWebinterfaceAddonsHandler.h"
#include "addons/AddonManager.h"

#define ADDON_HEADER      "<html><head><title>Add-on List</title></head><body>\n<h1>Available web interfaces:</h1>\n<ul>\n"

using namespace std;
using namespace ADDON;

bool CHTTPWebinterfaceAddonsHandler::CheckHTTPRequest(struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version)
{
  return (url.compare("/addons") == 0 || url.compare("/addons/") == 0);
}

#if (MHD_VERSION >= 0x00040001)
int CHTTPWebinterfaceAddonsHandler::HandleHTTPRequest(struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version,
                            const char *upload_data, size_t *upload_data_size, void **con_cls)
#else
int CHTTPWebinterfaceAddonsHandler::HandleHTTPRequest(struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version,
                            const char *upload_data, unsigned int *upload_data_size, void **con_cls)
#endif
{
  m_response = ADDON_HEADER;
  VECADDONS addons;
  CAddonMgr::Get().GetAddons(ADDON_WEB_INTERFACE, addons);
  IVECADDONS addons_it;
  for (addons_it=addons.begin(); addons_it!=addons.end(); addons_it++)
    m_response += "<li><a href=/addons/"+ (*addons_it)->ID() + "/>" + (*addons_it)->Name() + "</a></li>\n";

  m_response += "</ul>\n</body></html>";

  m_responseType = HTTPMemoryDownloadNoFreeCopy;
  m_responseCode = MHD_HTTP_OK;

  return MHD_YES;
}


