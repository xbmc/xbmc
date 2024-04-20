/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureOperations.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "TextureDatabase.h"
#include "utils/Variant.h"

#include <algorithm>

using namespace JSONRPC;

JSONRPC_STATUS CTextureOperations::GetTextures(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CFileItemList listItems;

  CTextureDatabase db;
  if (!db.Open())
    return InternalError;

  CDatabase::Filter dbFilter;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isObject())
  {
    CVariant xspObj(CVariant::VariantTypeObject);

    if (filter.isMember("field"))
    {
      xspObj["and"] = CVariant(CVariant::VariantTypeArray);
      xspObj["and"].push_back(filter);
    }
    else
      xspObj = filter;

    // decipher the rules
    CDatabaseQueryRuleCombination rule;
    if (!rule.Load(xspObj, &db))
      return InvalidParams;

    dbFilter.AppendWhere(rule.GetWhereClause(db, ""));
  }

  // fetch textures from the database
  CVariant items = CVariant(CVariant::VariantTypeArray);
  if (!db.GetTextures(items, dbFilter))
    return InternalError;

  // return only what was asked for, plus textureid
  CVariant prop = parameterObject["properties"];
  prop.push_back("textureid");
  if (!items.empty() && prop.isArray())
  {
    std::set<std::string> fields;
    CVariant &item = items[0];
    for (CVariant::const_iterator_map field = item.begin_map(); field != item.end_map(); ++field)
    {
      if (std::find(prop.begin_array(), prop.end_array(), field->first) == prop.end_array())
        fields.insert(field->first);
    }
    // erase these fields
    for (CVariant::iterator_array item = items.begin_array(); item != items.end_array(); ++item)
    {
      for (const auto& i : fields)
        item->erase(i);
    }
    if (fields.find("url") == fields.end())
    {
      // wrap cached url to something retrieval from Files.GetFiles()
      for (CVariant::iterator_array item = items.begin_array(); item != items.end_array(); ++item)
      {
        CVariant &cachedUrl = (*item)["url"];
        cachedUrl = CTextureUtils::GetWrappedImageURL(cachedUrl.asString());
      }
    }
  }

  result["textures"] = items;
  return OK;
}

JSONRPC_STATUS CTextureOperations::RemoveTexture(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["textureid"].asInteger();

  if (!CServiceBroker::GetTextureCache()->ClearCachedImage(id))
    return InvalidParams;

  return ACK;
}
