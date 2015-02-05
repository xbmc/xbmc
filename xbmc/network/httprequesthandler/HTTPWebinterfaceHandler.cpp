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
#include "addons/Webinterface.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#define WEBSERVER_DIRECTORY_SEPARATOR "/"

CHTTPWebinterfaceHandler::CHTTPWebinterfaceHandler(const HTTPRequest &request)
  : CHTTPFileHandler(request)
{
  // resolve the URL into a file path and a HTTP response status
  std::string file;
  int responseStatus = ResolveUrl(request.pathUrl, file);

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
  // determine the addon and addon's path
  if (!ResolveAddon(url, addon, path))
    return MHD_HTTP_NOT_FOUND;
  
  if (XFILE::CDirectory::Exists(path))
  {
    if (URIUtils::GetFileName(path).empty())
    {
      // determine the actual file path using the default entry point
      if (addon != NULL && addon->Type() == ADDON::ADDON_WEB_INTERFACE)
        path = std::dynamic_pointer_cast<ADDON::CWebinterface>(addon)->GetEntryPoint(path);
    }
    else
    {
      URIUtils::AddSlashAtEnd(path);
      return MHD_HTTP_FOUND;
    }
  }

  if (!XFILE::CFile::Exists(path))
    return MHD_HTTP_NOT_FOUND;

  return MHD_HTTP_OK;
}

bool CHTTPWebinterfaceHandler::ResolveAddon(const std::string &url, ADDON::AddonPtr &addon)
{
  std::string addonPath;
  return ResolveAddon(url, addon, addonPath);
}

bool CHTTPWebinterfaceHandler::ResolveAddon(const std::string &url, ADDON::AddonPtr &addon, std::string &addonPath)
{
  std::string path = url;

  // check if the URL references a specific addon
  if (url.find("/addons/") == 0 && url.size() > 8)
  {
    std::vector<std::string> components;
    StringUtils::Tokenize(path, components, WEBSERVER_DIRECTORY_SEPARATOR);
    if (components.size() <= 1)
      return false;

    if (!ADDON::CAddonMgr::Get().GetAddon(components.at(1), addon) || addon == NULL)
      return false;

    addonPath = addon->Path();
    if (addon->Type() != ADDON::ADDON_WEB_INTERFACE) // No need to append /htdocs for web interfaces
      addonPath = URIUtils::AddFileToFolder(addonPath, "/htdocs/");

    // remove /addons/<addon-id> from the path
    components.erase(components.begin(), components.begin() + 2);

    // determine the path within the addon
    path = StringUtils::Join(components, WEBSERVER_DIRECTORY_SEPARATOR);
  }
  else if (!ADDON::CAddonMgr::Get().GetDefault(ADDON::ADDON_WEB_INTERFACE, addon) || addon == NULL)
    return false;

  // get the path of the addon
  addonPath = addon->Path();

  // add /htdocs/ to the addon's path if it's not a webinterface
  if (addon->Type() != ADDON::ADDON_WEB_INTERFACE)
    addonPath = URIUtils::AddFileToFolder(addonPath, "/htdocs/");

  // append the path within the addon to the path of the addon
  addonPath = URIUtils::AddFileToFolder(addonPath, path);

  return true;
}
