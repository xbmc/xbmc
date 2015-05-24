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

#include "FavouritesOperations.h"
#include "filesystem/FavouritesDirectory.h"
#include "input/ButtonTranslator.h"
#include "utils/StringUtils.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "guilib/WindowIDs.h"
#include <vector>

using namespace std;
using namespace JSONRPC;
using namespace XFILE;

JSONRPC_STATUS CFavouritesOperations::GetFavourites(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CFileItemList favourites;
  CFavouritesDirectory::Load(favourites);
  
  string type = !parameterObject["type"].isNull() ? parameterObject["type"].asString() : "";

  set<string> fields;
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
    vector<string> parameters;
    CUtil::SplitExecFunction(item->GetPath(), function, parameters);
    if (parameters.size() == 0)
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
          object["window"] = CButtonTranslator::TranslateWindow(strtol(parameters[0].c_str(), NULL, 10));
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
  string type = parameterObject["type"].asString();

  if (type.compare("unknown") == 0)
    return InvalidParams;

  if ((type.compare("media") == 0 || type.compare("script") == 0) && !ParameterNotNull(parameterObject, "path"))
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

  string title = parameterObject["title"].asString();
  string path = parameterObject["path"].asString();

  CFileItem item;
  int contextWindow = 0;
  if (type.compare("window") == 0)
  {
    item = CFileItem(parameterObject["windowparameter"].asString(), true);
    contextWindow = CButtonTranslator::TranslateWindow(parameterObject["window"].asString());
    if (contextWindow == WINDOW_INVALID)
      return InvalidParams;
  } 
  else if (type.compare("script") == 0) 
  {
    if (!URIUtils::IsScript(path))
      path = "script://" + path;
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

  if (CFavouritesDirectory::AddOrRemove(&item, contextWindow))
    return ACK;
  else
    return FailedToExecute;
}
