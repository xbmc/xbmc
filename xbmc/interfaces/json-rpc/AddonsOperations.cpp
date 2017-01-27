/*
 *      Copyright (C) 2011-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonsOperations.h"
#include "JSONUtils.h"
#include "addons/AddonManager.h"
#include "addons/AddonDatabase.h"
#include "messaging/ApplicationMessenger.h"
#include "TextureCache.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace JSONRPC;
using namespace ADDON;
using namespace XFILE;
using namespace KODI::MESSAGING;

JSONRPC_STATUS CAddonsOperations::GetAddons(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::vector<TYPE> addonTypes;
  TYPE addonType = CAddonInfo::TranslateType(parameterObject["type"].asString());
  CAddonInfo::SubContent content = CAddonInfo::TranslateSubContent(parameterObject["content"].asString());
  CVariant enabled = parameterObject["enabled"];
  CVariant installed = parameterObject["installed"];

  // ignore the "content" parameter if the type is specified but not a plugin or script
  if (addonType != ADDON_UNKNOWN && addonType != ADDON_PLUGIN && addonType != ADDON_SCRIPT)
    content = CAddonInfo::UNKNOWN;

  bool contentFound = false;
  switch (addonType)
  {
  case ADDON_VIDEO:
    content = CAddonInfo::VIDEO;
    contentFound = true;
    break;
  case ADDON_AUDIO:
    content = CAddonInfo::AUDIO;
    contentFound = true;
    break;
  case ADDON_IMAGE:
    content = CAddonInfo::IMAGE;
    contentFound = true;
    break;
  case ADDON_GAME:
    content = CAddonInfo::GAME;
    contentFound = true;
    break;
  case ADDON_EXECUTABLE:
    content = CAddonInfo::EXECUTABLE;
    contentFound = true;
    break;

  default:
    break;
  }

  if (contentFound)
  {
    addonTypes.push_back(ADDON_PLUGIN);
    addonTypes.push_back(ADDON_SCRIPT);
  }
  else
    addonTypes.push_back(addonType);

   AddonInfos addons;
   for (const auto typeIt : addonTypes)
   {
     AddonInfos typeAddons;
     if (typeIt == ADDON_UNKNOWN)
     {
       if (!enabled.isBoolean()) //All
       {
//         if (!installed.isBoolean() || installed.asBoolean())
//           typeAddons = CAddonMgr::GetInstance().GetAddonInfos(false, ADDON_UNKNOWN);
//           CAddonMgr::GetInstance().GetInstalledAddons(typeAddons);
         if (!installed.isBoolean() || (installed.isBoolean() && !installed.asBoolean()))
           CAddonMgr::GetInstance().GetInstallableAddons(typeAddons);
       }
//       else if (enabled.asBoolean() && (!installed.isBoolean() || installed.asBoolean())) //Enabled
//         CAddonMgr::GetInstance().GetAddons(typeAddons);
//       else if (!installed.isBoolean() || installed.asBoolean())
//         CAddonMgr::GetInstance().GetDisabledAddons(typeAddons);
     }
     else
     {
       if (!enabled.isBoolean()) //All
       {
//         if (!installed.isBoolean() || installed.asBoolean())
//           CAddonMgr::GetInstance().GetInstalledAddons(typeAddons, *typeIt);
         if (!installed.isBoolean() || (installed.isBoolean() && !installed.asBoolean()))
           CAddonMgr::GetInstance().GetInstallableAddons(typeAddons, typeIt);
       }
//       else if (enabled.asBoolean() && (!installed.isBoolean() || installed.asBoolean())) //Enabled
//         CAddonMgr::GetInstance().GetAddons(typeAddons, *typeIt);
//       else if (!installed.isBoolean() || installed.asBoolean())
//         CAddonMgr::GetInstance().GetDisabledAddons(typeAddons, *typeIt);
     }

     addons.insert(addons.end(), typeAddons.begin(), typeAddons.end());
   }

   // remove library addons
   for (int index = 0; index < (int)addons.size(); index++)
   {
     AddonInfoPtr addonInfo = addons.at(index);
     if ((addons.at(index)->Type() <= ADDON_UNKNOWN || addons.at(index)->Type() >= ADDON_MAX) ||
         !addonInfo->ProvidesSubContent(content))
     {
       addons.erase(addons.begin() + index);
       index--;
     }
   }

   int start, end;
   HandleLimits(parameterObject, result, addons.size(), start, end);

   CAddonDatabase addondb;
//   for (int index = start; index < end; index++)
//     FillDetails(addons.at(index), parameterObject["properties"], result["addons"], addondb, true);

  return OK;
}

JSONRPC_STATUS CAddonsOperations::GetAddonDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string id = parameterObject["addonid"].asString();
  AddonPtr addon;
  if (!CAddonMgr::GetInstance().GetAddon(id, addon, ADDON::ADDON_UNKNOWN, false) || addon.get() == NULL ||
      addon->Type() <= ADDON_UNKNOWN || addon->Type() >= ADDON_MAX)
    return InvalidParams;
    
  CAddonDatabase addondb;
  FillDetails(addon, parameterObject["properties"], result["addon"], addondb);

  return OK;
}

JSONRPC_STATUS CAddonsOperations::SetAddonEnabled(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string id = parameterObject["addonid"].asString();
  AddonPtr addon;
  if (!CAddonMgr::GetInstance().GetAddon(id, addon, ADDON::ADDON_UNKNOWN, false) || addon == nullptr ||
    addon->Type() <= ADDON_UNKNOWN || addon->Type() >= ADDON_MAX)
    return InvalidParams;

  bool disabled = false;
  if (parameterObject["enabled"].isBoolean())
    disabled = !parameterObject["enabled"].asBoolean();
  // we need to toggle the current disabled state of the addon
  else if (parameterObject["enabled"].isString())
    disabled = CAddonMgr::GetInstance().IsAddonEnabled(id);
  else
    return InvalidParams;

  bool success = disabled ? CAddonMgr::GetInstance().DisableAddon(id) : CAddonMgr::GetInstance().EnableAddon(id);
  return success ? ACK : InvalidParams;
}

JSONRPC_STATUS CAddonsOperations::ExecuteAddon(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string id = parameterObject["addonid"].asString();
  AddonPtr addon;
  if (!CAddonMgr::GetInstance().GetAddon(id, addon) || addon.get() == NULL ||
      addon->Type() < ADDON_VIZ || addon->Type() >= ADDON_MAX)
    return InvalidParams;
    
  std::string argv;
  CVariant params = parameterObject["params"];
  if (params.isObject())
  {
    for (CVariant::const_iterator_map it = params.begin_map(); it != params.end_map(); it++)
    {
      if (it != params.begin_map())
        argv += ",";
      argv += it->first + "=" + it->second.asString();
    }
  }
  else if (params.isArray())
  {
    for (CVariant::const_iterator_array it = params.begin_array(); it != params.end_array(); it++)
    {
      if (it != params.begin_array())
        argv += ",";
      argv += StringUtils::Paramify(it->asString());
    }
  }
  else if (params.isString())
  {
    if (!params.empty())
      argv = StringUtils::Paramify(params.asString());
  }
  
  std::string cmd;
  if (params.empty())
    cmd = StringUtils::Format("RunAddon(%s)", id.c_str());
  else
    cmd = StringUtils::Format("RunAddon(%s, %s)", id.c_str(), argv.c_str());

  if (params["wait"].asBoolean())
    CApplicationMessenger::GetInstance().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, cmd);
  else
    CApplicationMessenger::GetInstance().PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, cmd);
  
  return ACK;
}

static CVariant Serialize(const AddonPtr& addon)
{
  CVariant variant;
  variant["addonid"] = addon->ID();
  variant["type"] = ADDON::CAddonInfo::TranslateType(addon->Type(), false);
  variant["name"] = addon->Name();
  variant["version"] = addon->Version().asString();
  variant["summary"] = addon->Summary();
  variant["description"] = addon->Description();
  variant["path"] = addon->Path();
  variant["author"] = addon->Author();
  variant["thumbnail"] = addon->Icon();
  variant["disclaimer"] = addon->Disclaimer();
  variant["fanart"] = addon->FanArt();

  variant["dependencies"] = CVariant(CVariant::VariantTypeArray);
  for (const auto& kv : addon->GetDeps())
  {
    CVariant dep(CVariant::VariantTypeObject);
    dep["addonid"] = kv.first;
    dep["version"] = kv.second.first.asString();
    dep["optional"] = kv.second.second;
    variant["dependencies"].push_back(std::move(dep));
  }
  if (addon->Broken().empty())
    variant["broken"] = false;
  else
    variant["broken"] = addon->Broken();
  /**
   * @warning only the base extended values from addon.xml are available here!
   * All others defined in child parts on xml are need to load separate.
   * @todo find a way to bring everything here?
   */
  variant["extrainfo"] = CVariant(CVariant::VariantTypeArray);
  for (auto values : addon->AddonInfo()->GetExtValues())
  {
    for (auto value : values.second)
    {
      CVariant info(CVariant::VariantTypeObject);
      info["key"] = value.first;
      info["value"] = value.second.asString();
      variant["extrainfo"].push_back(std::move(info));
    }
  }
  variant["rating"] = -1;
  return variant;
}

void CAddonsOperations::FillDetails(AddonPtr addon, const CVariant& fields, CVariant &result, CAddonDatabase &addondb, bool append /* = false */)
{
  if (addon.get() == NULL)
    return;
  
  CVariant addonInfo = Serialize(addon);

  CVariant object;
  object["addonid"] = addonInfo["addonid"];
  object["type"] = addonInfo["type"];
  
  for (unsigned int index = 0; index < fields.size(); index++)
  {
    std::string field = fields[index].asString();
    
    // we need to manually retrieve the enabled / installed state of every addon
    // from the addon database because it can't be read from addon.xml
    if (field == "enabled")
    {
      object[field] = CAddonMgr::GetInstance().IsAddonEnabled(addon->ID());
    }
    else if (field == "installed")
    {
      object[field] = CAddonMgr::GetInstance().IsAddonInstalled(addon->ID());
    }
    else if (field == "fanart" || field == "thumbnail")
    {
      std::string url = addonInfo[field].asString();
      // We need to check the existence of fanart and thumbnails as the addon simply
      // holds where the art will be, not whether it exists.
      bool needsRecaching;
      std::string image = CTextureCache::GetInstance().CheckCachedImage(url, needsRecaching);
      if (!image.empty() || CFile::Exists(url))
        object[field] = CTextureUtils::GetWrappedImageURL(url);
      else
        object[field] = "";
    }
    else if (addonInfo.isMember(field))
      object[field] = addonInfo[field];
  }
  
  if (append)
    result.append(object);
  else
    result = object;
}
