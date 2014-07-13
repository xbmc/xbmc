/*
 *      Copyright (C) 2013 Team XBMC
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

#include "TextureOperations.h"
#include "TextureDatabase.h"
#include "TextureCache.h"

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
      for (std::set<std::string>::const_iterator i = fields.begin(); i != fields.end(); ++i)
        item->erase(*i);
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

  if (!CTextureCache::Get().ClearCachedImage(id))
    return InvalidParams;

  return ACK;
}
