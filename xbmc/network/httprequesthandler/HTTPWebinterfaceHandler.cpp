/*
 *      Copyright (C) 2011-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "HTTPWebinterfaceHandler.h"
#include "network/WebServer.h"
#include "addons/AddonManager.h"
#include "utils/URIUtils.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "Util.h"

#define DEFAULT_PAGE        "index.html"

using namespace std;
using namespace ADDON;
using namespace XFILE;

bool CHTTPWebinterfaceHandler::CheckHTTPRequest(const HTTPRequest &request)
{
  return true;
}

int CHTTPWebinterfaceHandler::HandleHTTPRequest(const HTTPRequest &request)
{
  m_responseCode = ResolveUrl(request.url, m_url);
  if (m_responseCode != MHD_HTTP_OK)
  {
    if (m_responseCode == MHD_HTTP_FOUND)
      m_responseType = HTTPRedirect;
    else
      m_responseType = HTTPError;

    return MHD_YES;
  }

  m_responseType = HTTPFileDownload;

  return MHD_YES;
}

int CHTTPWebinterfaceHandler::ResolveUrl(const std::string &url, std::string &path)
{
  AddonPtr dummyAddon;
  return ResolveUrl(url, path, dummyAddon);
}

int CHTTPWebinterfaceHandler::ResolveUrl(const std::string &url, std::string &path, AddonPtr &addon)
{
  string addonPath;
  bool useDefaultWebInterface = true;

  path = url;
  if (url.find("/addons/") == 0 && url.size() > 8)
  {
    CStdStringArray components;
    CUtil::Tokenize(path, components, "/");
    if (components.size() > 1)
    {
      CAddonMgr::Get().GetAddon(components.at(1), addon);
      if (addon)
      {
        size_t pos;
        pos = path.find('/', 8); // /addons/ = 8 characters +1 to start behind the last slash
        if (pos != string::npos)
          path = path.substr(pos);
        else // missing trailing slash
        {
          path = url + "/";
          return MHD_HTTP_FOUND;
        }

        useDefaultWebInterface = false;
        addonPath = addon->Path();
        if (addon->Type() != ADDON_WEB_INTERFACE) // No need to append /htdocs for web interfaces
          addonPath = URIUtils::AddFileToFolder(addonPath, "/htdocs/");
      }
    }
    else
      return MHD_HTTP_NOT_FOUND;
  }

  if (path.compare("/") == 0)
    path.append(DEFAULT_PAGE);

  if (useDefaultWebInterface)
  {
    CAddonMgr::Get().GetDefault(ADDON_WEB_INTERFACE, addon);
    if (addon)
      addonPath = addon->Path();
  }

  if (addon)
    path = URIUtils::AddFileToFolder(addonPath, path);

  string realPath = URIUtils::GetRealPath(path);
  string realAddonPath = URIUtils::GetRealPath(addonPath);
  if (!URIUtils::IsInPath(realPath, realAddonPath))
    return MHD_HTTP_NOT_FOUND;
  
  if (CDirectory::Exists(path))
  {
    if (path.at(path.size() -1) == '/')
      path.append(DEFAULT_PAGE);
    else
    {
      path = url + "/";
      return MHD_HTTP_FOUND;
    }
  }

  if (!CFile::Exists(path))
    return MHD_HTTP_NOT_FOUND;

  return MHD_HTTP_OK;
}
