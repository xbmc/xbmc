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

#include "HTTPWebinterfaceHandler.h"
#include "addons/AddonManager.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#define DEFAULT_PAGE        "index.html"

CHTTPWebinterfaceHandler::CHTTPWebinterfaceHandler(const HTTPRequest &request)
  : CHTTPFileHandler(request)
{
  // resolve the URL into a file path and a HTTP response status
  std::string file;
  int responseStatus = ResolveUrl(request.url, file);

  // set the file and the HTTP response status
  SetFile(file, responseStatus);
}

bool CHTTPWebinterfaceHandler::CanHandleRequest(const HTTPRequest &request)
{
  return true;
}

int CHTTPWebinterfaceHandler::ResolveUrl(const std::string &url, std::string &path)
{
  ADDON::AddonPtr dummyAddon;
  return ResolveUrl(url, path, dummyAddon);
}

int CHTTPWebinterfaceHandler::ResolveUrl(const std::string &url, std::string &path, ADDON::AddonPtr &addon)
{
  std::string addonPath;
  bool useDefaultWebInterface = true;

  path = url;
  if (url.find("/addons/") == 0 && url.size() > 8)
  {
    std::vector<std::string> components;
    StringUtils::Tokenize(path, components, "/");
    if (components.size() > 1)
    {
      ADDON::CAddonMgr::Get().GetAddon(components.at(1), addon);
      if (addon)
      {
        size_t pos;
        pos = path.find('/', 8); // /addons/ = 8 characters +1 to start behind the last slash
        if (pos != std::string::npos)
          path = path.substr(pos);
        else // missing trailing slash
        {
          path = url + "/";
          return MHD_HTTP_FOUND;
        }

        useDefaultWebInterface = false;
        addonPath = addon->Path();
        if (addon->Type() != ADDON::ADDON_WEB_INTERFACE) // No need to append /htdocs for web interfaces
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
    ADDON::CAddonMgr::Get().GetDefault(ADDON::ADDON_WEB_INTERFACE, addon);
    if (addon)
      addonPath = addon->Path();
  }

  if (addon)
    path = URIUtils::AddFileToFolder(addonPath, path);

  std::string realPath = URIUtils::GetRealPath(path);
  std::string realAddonPath = URIUtils::GetRealPath(addonPath);
  if (!URIUtils::IsInPath(realPath, realAddonPath))
    return MHD_HTTP_NOT_FOUND;
  
  if (XFILE::CDirectory::Exists(path))
  {
    if (path.at(path.size() -1) == '/')
      path.append(DEFAULT_PAGE);
    else
    {
      path = url + "/";
      return MHD_HTTP_FOUND;
    }
  }

  if (!XFILE::CFile::Exists(path))
    return MHD_HTTP_NOT_FOUND;

  return MHD_HTTP_OK;
}
