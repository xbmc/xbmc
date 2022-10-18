/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HTTPWebinterfaceHandler.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "addons/Webinterface.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/Directory.h"
#include "utils/FileUtils.h"
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

bool CHTTPWebinterfaceHandler::CanHandleRequest(const HTTPRequest &request) const
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
      if (addon != NULL && addon->Type() == ADDON::AddonType::WEB_INTERFACE)
        path = std::dynamic_pointer_cast<ADDON::CWebinterface>(addon)->GetEntryPoint(path);
    }
    else
    {
      URIUtils::AddSlashAtEnd(path);
      return MHD_HTTP_FOUND;
    }
  }

  if (!CFileUtils::CheckFileAccessAllowed(path))
    return MHD_HTTP_NOT_FOUND;

  if (!CFileUtils::Exists(path))
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

    if (!CServiceBroker::GetAddonMgr().GetAddon(components.at(1), addon,
                                                ADDON::OnlyEnabled::CHOICE_YES) ||
        addon == NULL)
      return false;

    addonPath = addon->Path();
    if (addon->Type() !=
        ADDON::AddonType::WEB_INTERFACE) // No need to append /htdocs for web interfaces
      addonPath = URIUtils::AddFileToFolder(addonPath, "/htdocs/");

    // remove /addons/<addon-id> from the path
    components.erase(components.begin(), components.begin() + 2);

    // determine the path within the addon
    path = StringUtils::Join(components, WEBSERVER_DIRECTORY_SEPARATOR);
  }
  else if (!ADDON::CAddonSystemSettings::GetInstance().GetActive(ADDON::AddonType::WEB_INTERFACE,
                                                                 addon) ||
           addon == NULL)
    return false;

  // get the path of the addon
  addonPath = addon->Path();

  // add /htdocs/ to the addon's path if it's not a webinterface
  if (addon->Type() != ADDON::AddonType::WEB_INTERFACE)
    addonPath = URIUtils::AddFileToFolder(addonPath, "/htdocs/");

  // append the path within the addon to the path of the addon
  addonPath = URIUtils::AddFileToFolder(addonPath, path);

  // ensure that we don't have a directory traversal hack here
  // by checking if the resolved absolute path is inside the addon path
  std::string realPath = URIUtils::GetRealPath(addonPath);
  std::string realAddonPath = URIUtils::GetRealPath(addon->Path());
  if (!URIUtils::PathHasParent(realPath, realAddonPath, true))
    return false;

  return true;
}
