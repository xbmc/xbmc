/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonsOperations.h"

#include "JSONUtils.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "addons/AddonManager.h"
#include "addons/PluginSource.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace JSONRPC;
using namespace ADDON;

JSONRPC_STATUS CAddonsOperations::GetAddons(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::vector<AddonType> addonTypes;
  AddonType addonType = CAddonInfo::TranslateType(parameterObject["type"].asString());
  CPluginSource::Content content = CPluginSource::Translate(parameterObject["content"].asString());
  CVariant enabled = parameterObject["enabled"];
  CVariant installed = parameterObject["installed"];

  // ignore the "content" parameter if the type is specified but not a plugin or script
  if (addonType != AddonType::UNKNOWN && addonType != AddonType::PLUGIN &&
      addonType != AddonType::SCRIPT)
    content = CPluginSource::UNKNOWN;

  if (addonType >= AddonType::VIDEO && addonType <= AddonType::EXECUTABLE)
  {
    addonTypes.push_back(AddonType::PLUGIN);
    addonTypes.push_back(AddonType::SCRIPT);

    switch (addonType)
    {
      case AddonType::VIDEO:
        content = CPluginSource::VIDEO;
        break;
      case AddonType::AUDIO:
        content = CPluginSource::AUDIO;
        break;
      case AddonType::IMAGE:
        content = CPluginSource::IMAGE;
        break;
      case AddonType::GAME:
        content = CPluginSource::GAME;
        break;
      case AddonType::EXECUTABLE:
        content = CPluginSource::EXECUTABLE;
        break;
      default:
        break;
    }
  }
  else
    addonTypes.push_back(addonType);

  VECADDONS addons;
  for (const auto& typeIt : addonTypes)
  {
    VECADDONS typeAddons;
    if (typeIt == AddonType::UNKNOWN)
    {
      if (!enabled.isBoolean()) //All
      {
        if (!installed.isBoolean() || installed.asBoolean())
          CServiceBroker::GetAddonMgr().GetInstalledAddons(typeAddons);
        if (!installed.isBoolean() || (installed.isBoolean() && !installed.asBoolean()))
          CServiceBroker::GetAddonMgr().GetInstallableAddons(typeAddons);
      }
      else if (enabled.asBoolean() && (!installed.isBoolean() || installed.asBoolean())) //Enabled
        CServiceBroker::GetAddonMgr().GetAddons(typeAddons);
      else if (!installed.isBoolean() || installed.asBoolean())
        CServiceBroker::GetAddonMgr().GetDisabledAddons(typeAddons);
    }
    else
    {
      if (!enabled.isBoolean()) //All
      {
        if (!installed.isBoolean() || installed.asBoolean())
          CServiceBroker::GetAddonMgr().GetInstalledAddons(typeAddons, typeIt);
        if (!installed.isBoolean() || (installed.isBoolean() && !installed.asBoolean()))
          CServiceBroker::GetAddonMgr().GetInstallableAddons(typeAddons, typeIt);
      }
      else if (enabled.asBoolean() && (!installed.isBoolean() || installed.asBoolean())) //Enabled
        CServiceBroker::GetAddonMgr().GetAddons(typeAddons, typeIt);
      else if (!installed.isBoolean() || installed.asBoolean())
        CServiceBroker::GetAddonMgr().GetDisabledAddons(typeAddons, typeIt);
    }

    addons.insert(addons.end(), typeAddons.begin(), typeAddons.end());
  }

  // remove library addons
  for (int index = 0; index < (int)addons.size(); index++)
  {
    std::shared_ptr<CPluginSource> plugin;
    if (content != CPluginSource::UNKNOWN)
      plugin = std::dynamic_pointer_cast<CPluginSource>(addons.at(index));

    if ((addons.at(index)->Type() <= AddonType::UNKNOWN ||
         addons.at(index)->Type() >= AddonType::MAX_TYPES) ||
        ((content != CPluginSource::UNKNOWN && plugin == NULL) ||
         (plugin != NULL && !plugin->Provides(content))))
    {
      addons.erase(addons.begin() + index);
      index--;
    }
  }

  int start, end;
  HandleLimits(parameterObject, result, addons.size(), start, end);

  for (int index = start; index < end; index++)
    FillDetails(addons.at(index), parameterObject["properties"], result["addons"], true);

  return OK;
}

JSONRPC_STATUS CAddonsOperations::GetAddonDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string id = parameterObject["addonid"].asString();
  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(id, addon, OnlyEnabled::CHOICE_NO) ||
      addon.get() == NULL || addon->Type() <= AddonType::UNKNOWN ||
      addon->Type() >= AddonType::MAX_TYPES)
    return InvalidParams;

  FillDetails(addon, parameterObject["properties"], result["addon"], false);

  return OK;
}

JSONRPC_STATUS CAddonsOperations::SetAddonEnabled(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string id = parameterObject["addonid"].asString();
  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(id, addon, OnlyEnabled::CHOICE_NO) ||
      addon == nullptr || addon->Type() <= AddonType::UNKNOWN ||
      addon->Type() >= AddonType::MAX_TYPES)
    return InvalidParams;

  bool disabled = false;
  if (parameterObject["enabled"].isBoolean())
  {
    disabled = !parameterObject["enabled"].asBoolean();
  }
  // we need to toggle the current disabled state of the addon
  else if (parameterObject["enabled"].isString())
  {
    disabled = !CServiceBroker::GetAddonMgr().IsAddonDisabled(id);
  }
  else
  {
    return InvalidParams;
  }

  bool success = disabled
                     ? CServiceBroker::GetAddonMgr().DisableAddon(id, AddonDisabledReason::USER)
                     : CServiceBroker::GetAddonMgr().EnableAddon(id);

  return success ? ACK : InvalidParams;
}

JSONRPC_STATUS CAddonsOperations::ExecuteAddon(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string id = parameterObject["addonid"].asString();
  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(id, addon, OnlyEnabled::CHOICE_YES) ||
      addon.get() == NULL || addon->Type() < AddonType::VISUALIZATION ||
      addon->Type() >= AddonType::MAX_TYPES)
    return InvalidParams;

  std::string argv;
  CVariant params = parameterObject["params"];
  if (params.isObject())
  {
    for (CVariant::const_iterator_map it = params.begin_map(); it != params.end_map(); ++it)
    {
      if (it != params.begin_map())
        argv += ",";
      argv += it->first + "=" + it->second.asString();
    }
  }
  else if (params.isArray())
  {
    for (CVariant::const_iterator_array it = params.begin_array(); it != params.end_array(); ++it)
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
    cmd = StringUtils::Format("RunAddon({})", id);
  else
    cmd = StringUtils::Format("RunAddon({}, {})", id, argv);

  if (params["wait"].asBoolean())
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, cmd);
  else
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, cmd);

  return ACK;
}

static CVariant Serialize(const AddonPtr& addon)
{
  CVariant variant;
  variant["addonid"] = addon->ID();
  variant["type"] = CAddonInfo::TranslateType(addon->Type(), false);
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
  for (const auto& dep : addon->GetDependencies())
  {
    CVariant info(CVariant::VariantTypeObject);
    info["addonid"] = dep.id;
    info["minversion"] = dep.versionMin.asString();
    info["version"] = dep.version.asString();
    info["optional"] = dep.optional;
    variant["dependencies"].push_back(std::move(info));
  }
  if (addon->LifecycleState() == AddonLifecycleState::BROKEN)
    variant["broken"] = addon->LifecycleStateDescription();
  else
    variant["broken"] = false;
  if (addon->LifecycleState() == AddonLifecycleState::DEPRECATED)
    variant["deprecated"] = addon->LifecycleStateDescription();
  else
    variant["deprecated"] = false;
  variant["extrainfo"] = CVariant(CVariant::VariantTypeArray);
  for (const auto& kv : addon->ExtraInfo())
  {
    CVariant info(CVariant::VariantTypeObject);
    info["key"] = kv.first;
    info["value"] = kv.second;
    variant["extrainfo"].push_back(std::move(info));
  }
  variant["rating"] = -1;
  return variant;
}

void CAddonsOperations::FillDetails(const std::shared_ptr<ADDON::IAddon>& addon,
                                    const CVariant& fields,
                                    CVariant& result,
                                    bool append)
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
      object[field] = !CServiceBroker::GetAddonMgr().IsAddonDisabled(addon->ID());
    }
    else if (field == "installed")
    {
      object[field] = CServiceBroker::GetAddonMgr().IsAddonInstalled(addon->ID());
    }
    else if (field == "fanart" || field == "thumbnail")
    {
      std::string url = addonInfo[field].asString();
      // We need to check the existence of fanart and thumbnails as the addon simply
      // holds where the art will be, not whether it exists.
      bool needsRecaching;
      std::string image = CServiceBroker::GetTextureCache()->CheckCachedImage(url, needsRecaching);
      if (!image.empty() || CFileUtils::Exists(url))
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
