/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FavouritesOperations.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "favourites/FavouritesService.h"
#include "guilib/WindowIDs.h"
#include "input/WindowTranslator.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include <vector>

using namespace JSONRPC;

JSONRPC_STATUS CFavouritesOperations::GetFavourites(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CFileItemList favourites;
  CServiceBroker::GetFavouritesService().GetAll(favourites);

  std::string type = !parameterObject["type"].isNull() ? parameterObject["type"].asString() : "";

  std::set<std::string> fields;
  if (parameterObject.isMember("properties") && parameterObject["properties"].isArray())
  {
    for (CVariant::const_iterator_array field = parameterObject["properties"].begin_array(); field != parameterObject["properties"].end_array(); field++)
      fields.insert(field->asString());
  }

  for (int i = 0; i < favourites.Size(); i++)
  {
    CVariant object;
    CFileItemPtr item = favourites.Get(i);

    std::string function;
    std::vector<std::string> parameters;

    //FIXME: this path is internal to the favourites system and should not be parsed and exposed
    CURL url(item->GetPath());
    std::string internalPath = CURL::Decode(url.GetHostName());

    CUtil::SplitExecFunction(internalPath, function, parameters);
    if (parameters.empty())
      continue;

    object["title"] = item->GetLabel();
    if (fields.find("thumbnail") !=  fields.end())
      object["thumbnail"] = item->GetArt("thumb");

    if (StringUtils::EqualsNoCase(function, "ActivateWindow"))
    {
      object["type"] = "window";
      if (fields.find("window") != fields.end())
      {
        if (StringUtils::IsNaturalNumber(parameters[0]))
          object["window"] = CWindowTranslator::TranslateWindow(strtol(parameters[0].c_str(), NULL, 10));
        else
          object["window"] = parameters[0];
      }
      if (fields.find("windowparameter") != fields.end())
      {
        if (parameters.size() > 1)
          object["windowparameter"] = parameters[1];
        else
          object["windowparameter"] = "";
      }
    }
    else if (StringUtils::EqualsNoCase(function, "PlayMedia"))
    {
      object["type"] = "media";
      if (fields.find("path") !=  fields.end())
        object["path"] = parameters[0];
    }
    else if (StringUtils::EqualsNoCase(function, "RunScript"))
    {
      object["type"] = "script";
      if (fields.find("path") !=  fields.end())
        object["path"] = parameters[0];
    }
    else if (StringUtils::EqualsNoCase(function, "StartAndroidActivity"))
    {
      object["type"] = "androidapp";
      if (fields.find("path") !=  fields.end())
        object["path"] = parameters[0];
    }
    else
      object["type"] = "unknown";

    if (type.empty() || type.compare(object["type"].asString()) == 0)
      result["favourites"].append(object);
  }

  int start, end;
  HandleLimits(parameterObject, result, result["favourites"].size(), start, end);

  return OK;
}

JSONRPC_STATUS CFavouritesOperations::AddFavourite(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string type = parameterObject["type"].asString();

  if (type.compare("unknown") == 0)
    return InvalidParams;

  if ((type.compare("media") == 0 || type.compare("script") == 0 || type.compare("androidapp") == 0) && !ParameterNotNull(parameterObject, "path"))
  {
    result["method"] = "Favourites.AddFavourite";
    result["stack"]["message"] = "Missing parameter";
    result["stack"]["name"] = "path";
    result["stack"]["type"] = "string";
    return InvalidParams;
  }

  if (type.compare("window") == 0 && !ParameterNotNull(parameterObject, "window"))
  {
    result["method"] = "Favourites.AddFavourite";
    result["stack"]["message"] = "Missing parameter";
    result["stack"]["name"] = "window";
    result["stack"]["type"] = "string";
    return InvalidParams;
  }

  std::string title = parameterObject["title"].asString();
  std::string path = parameterObject["path"].asString();

  CFileItem item;
  int contextWindow = 0;
  if (type.compare("window") == 0)
  {
    item = CFileItem(parameterObject["windowparameter"].asString(), true);
    contextWindow = CWindowTranslator::TranslateWindow(parameterObject["window"].asString());
    if (contextWindow == WINDOW_INVALID)
      return InvalidParams;
  }
  else if (type.compare("script") == 0)
  {
    if (!URIUtils::IsScript(path))
      path = "script://" + path;
    item = CFileItem(path, false);
  }
  else if (type.compare("androidapp") == 0)
  {
    if (!URIUtils::IsAndroidApp(path))
      path = "androidapp://" + path;
    item = CFileItem(path, false);
  }
  else if (type.compare("media") == 0)
  {
    item = CFileItem(path, false);
  }
  else
    return InvalidParams;

  item.SetLabel(title);
  if (ParameterNotNull(parameterObject,"thumbnail"))
    item.SetArt("thumb", parameterObject["thumbnail"].asString());

  if (CServiceBroker::GetFavouritesService().AddOrRemove(item, contextWindow))
    return ACK;
  else
    return FailedToExecute;
}
