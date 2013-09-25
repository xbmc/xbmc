/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "ContactLibrary.h"
#include "contacts/ContactDatabase.h"
#include "FileItem.h"
#include "Util.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "ApplicationMessenger.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "TextureCache.h"
#include "settings/MediaSourceSettings.h"
#include "utils/log.h"

//using namespace CONTACT_INFO;
using namespace JSONRPC;
using namespace XFILE;


JSONRPC_STATUS CContactLibrary::GetContacts(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
    CContactDatabase contactdatabase;
    if (!contactdatabase.Open())
        return InternalError;
    
    CContactDbUrl contactUrl;
    contactUrl.FromString("contactdb://albums/");
    int artistID = -1, genreID = -1;
    const CVariant &filter = parameterObject["filter"];
    if (filter.isMember("artistid"))
        artistID = (int)filter["artistid"].asInteger();
    else if (filter.isMember("artist"))
        contactUrl.AddOption("artist", filter["artist"].asString());
    else if (filter.isMember("genreid"))
        genreID = (int)filter["genreid"].asInteger();
    else if (filter.isMember("genre"))
        contactUrl.AddOption("genre", filter["genre"].asString());
    else if (filter.isObject())
    {
        CStdString xsp;
        if (!GetXspFiltering("albums", filter, xsp))
            return InvalidParams;
        
        contactUrl.AddOption("xsp", xsp);
    }
    SortDescription sorting;
  /*
    ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
    if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
        return InvalidParams;
    */
    CFileItemList items;
  
    if (!contactdatabase.GetContactsNav(contactUrl.ToString(), items, sorting))
        return InternalError;
    
    JSONRPC_STATUS ret = GetAdditionalContactDetails(parameterObject, items, contactdatabase);
    if (ret != OK)
        return ret;
    
    int size = items.Size();
    if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
        size = (int)items.GetProperty("total").asInteger();
    HandleFileItemList("contactid", false, "contacts", items, parameterObject, result, size, false);
    
    return OK;
}

bool CContactLibrary::CheckForAdditionalProperties(const CVariant &properties, const std::set<std::string> &checkProperties, std::set<std::string> &foundProperties)
{
  if (!properties.isArray() || properties.empty())
    return false;
  
  std::set<std::string> checkingProperties = checkProperties;
  for (CVariant::const_iterator_array itr = properties.begin_array(); itr != properties.end_array() && !checkingProperties.empty(); itr++)
  {
    if (!itr->isString())
      continue;
    
    std::string property = itr->asString();
    if (checkingProperties.find(property) != checkingProperties.end())
    {
      checkingProperties.erase(property);
      foundProperties.insert(property);
    }
  }
  
  return !foundProperties.empty();
}

JSONRPC_STATUS CContactLibrary::GetAdditionalContactDetails(const CVariant &parameterObject, CFileItemList &items, CContactDatabase &contactdatabase)
{
  if (!contactdatabase.Open())
    return InternalError;
  
  std::set<std::string> checkProperties;
  checkProperties.insert("locationid");
  checkProperties.insert("faceid");
  checkProperties.insert("albumfaceid");
  std::set<std::string> additionalProperties;
  if (!CheckForAdditionalProperties(parameterObject["properties"], checkProperties, additionalProperties))
    return OK;
  
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    /*
    if (additionalProperties.find("locationid") != additionalProperties.end())
    {
      std::vector<int> locationids;
      if (contactdatabase.GetLocationsByContact(item->GetContactInfoTag()->GetDatabaseId(), locationids))
      {
        CVariant locationidObj(CVariant::VariantTypeArray);
        for (std::vector<int>::const_iterator locationid = locationids.begin(); locationid != locationids.end(); ++locationid)
          locationidObj.push_back(*locationid);
        
        item->SetProperty("locationid", locationidObj);
      }
    }
    if (additionalProperties.find("faceid") != additionalProperties.end())
    {
      std::vector<int> faceids;
      if (contactdatabase.GetFacesByContact(item->GetContactInfoTag()->GetDatabaseId(), true, faceids))
      {
        CVariant faceidObj(CVariant::VariantTypeArray);
        for (std::vector<int>::const_iterator faceid = faceids.begin(); faceid != faceids.end(); ++faceid)
          faceidObj.push_back(*faceid);
        
        item->SetProperty("faceid", faceidObj);
      }
    }
    if (additionalProperties.find("albumfaceid") != additionalProperties.end() && item->GetContactInfoTag()->GetContactAlbumId() > 0)
    {
      std::vector<int> albumfaceids;
      if (contactdatabase.GetFacesByContactAlbum(item->GetContactInfoTag()->GetContactAlbumId(), true, albumfaceids))
      {
        CVariant albumfaceidObj(CVariant::VariantTypeArray);
        for (std::vector<int>::const_iterator albumfaceid = albumfaceids.begin(); albumfaceid != albumfaceids.end(); ++albumfaceid)
          albumfaceidObj.push_back(*albumfaceid);
        
        item->SetProperty("albumfaceid", albumfaceidObj);
      }
    }
     */
  }
  
  return OK;
}
JSONRPC_STATUS CContactLibrary::GetContactDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
    int albumID = (int)parameterObject["albumid"].asInteger();
    
    CContactDatabase contactdatabase;
    if (!contactdatabase.Open())
        return InternalError;
  
    return OK;
}

JSONRPC_STATUS CContactLibrary::AddContact(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CContactDatabase contactdatabase;
  if (!contactdatabase.Open())
    return InternalError;
  
  //take the first source as the path
  VECSOURCES *shares = CMediaSourceSettings::Get().GetSources("pictures");
  if( !shares )
    return InternalError;
  
  const CVariant &name = parameterObject["name"];
  CStdString strFirst = name["first"].asString().c_str();
  CStdString strLast = name["last"].asString().c_str();
  
  CLog::Log(LOGINFO, "JSONRPC: Adding %s %s \n", strFirst.c_str(), strLast.c_str());
  //create the thumbnail and pass it in
  CStdString strContactPath = shares->at(0).strPath + strFirst + strLast + ".JPG";
  
  const CVariant &phones = parameterObject["phones"];
  std::map<std::string, std::string> phoneValues;
  phoneValues.insert(std::map<std::string, std::string>::value_type("mobile", phones["mobile"].asString().c_str()));
  phoneValues.insert(std::map<std::string, std::string>::value_type("work", phones["work"].asString().c_str()));
  phoneValues.insert(std::map<std::string, std::string>::value_type("home", phones["home"].asString().c_str()));

  const CVariant &email = parameterObject["emails"];
  std::map<std::string, std::string> emailValues;
  emailValues.insert(std::map<std::string, std::string>::value_type("email1", email["email1"].asString().c_str()));
  emailValues.insert(std::map<std::string, std::string>::value_type("email2", email["email2"].asString().c_str()));
  emailValues.insert(std::map<std::string, std::string>::value_type("email3", email["email3"].asString().c_str()));
  emailValues.insert(std::map<std::string, std::string>::value_type("email4", email["email4"].asString().c_str()));
  emailValues.insert(std::map<std::string, std::string>::value_type("email5", email["email5"].asString().c_str()));

  const CVariant &prof = parameterObject["profession"];
  std::map<std::string, std::string> profValues;
  profValues.insert(std::map<std::string, std::string>::value_type("organization", prof["organization"].asString().c_str()));
  profValues.insert(std::map<std::string, std::string>::value_type("title", prof["title"].asString().c_str()));
  profValues.insert(std::map<std::string, std::string>::value_type("department", prof["department"].asString().c_str()));

  const CVariant &home = parameterObject["home"];
  std::map<std::string, std::string> homeValues;
  homeValues.insert(std::map<std::string, std::string>::value_type("houseno", home["houseno"].asString().c_str()));
  homeValues.insert(std::map<std::string, std::string>::value_type("floor", home["floor"].asString().c_str()));
  homeValues.insert(std::map<std::string, std::string>::value_type("street", home["street"].asString().c_str()));
  homeValues.insert(std::map<std::string, std::string>::value_type("city", home["city"].asString().c_str()));
  homeValues.insert(std::map<std::string, std::string>::value_type("state", home["state"].asString().c_str()));
  homeValues.insert(std::map<std::string, std::string>::value_type("zip", home["zip"].asString().c_str()));
  homeValues.insert(std::map<std::string, std::string>::value_type("country", home["country"].asString().c_str()));
  homeValues.insert(std::map<std::string, std::string>::value_type("countrycode", home["countrycode"].asString().c_str()));

  const CVariant &work = parameterObject["work"];
  std::map<std::string, std::string> workValues;
  workValues.insert(std::map<std::string, std::string>::value_type("houseno", work["houseno"].asString().c_str()));
  workValues.insert(std::map<std::string, std::string>::value_type("floor", work["floor"].asString().c_str()));
  workValues.insert(std::map<std::string, std::string>::value_type("street", work["street"].asString().c_str()));
  workValues.insert(std::map<std::string, std::string>::value_type("city", work["city"].asString().c_str()));
  workValues.insert(std::map<std::string, std::string>::value_type("state", work["state"].asString().c_str()));
  workValues.insert(std::map<std::string, std::string>::value_type("zip", work["zip"].asString().c_str()));
  workValues.insert(std::map<std::string, std::string>::value_type("country", work["country"].asString().c_str()));
  workValues.insert(std::map<std::string, std::string>::value_type("countrycode", work["countrycode"].asString().c_str()));

  std::map<std::string, std::string> nameValues;
  nameValues.insert(std::map<std::string, std::string>::value_type("first", name["first"].asString().c_str()));
  nameValues.insert(std::map<std::string, std::string>::value_type("middle", name["middle"].asString().c_str()));
  nameValues.insert(std::map<std::string, std::string>::value_type("last", name["last"].asString().c_str()));
  nameValues.insert(std::map<std::string, std::string>::value_type("nick", name["nick"].asString().c_str()));
  nameValues.insert(std::map<std::string, std::string>::value_type("prefix", name["prefix"].asString().c_str()));
  nameValues.insert(std::map<std::string, std::string>::value_type("suffix", name["suffix"].asString().c_str()));
  nameValues.insert(std::map<std::string, std::string>::value_type("note", name["note"].asString().c_str()));

  std::map<std::string, std::string> dateValues;
  std::map<std::string, std::string> relValues;
  std::map<std::string, std::string> IMValues;
    std::map<std::string, std::string> URLValues;
  CLog::Log(LOGINFO, "JSONRPC: Adding  '%s' '%s'\n", nameValues["first"].c_str(), nameValues["last"].c_str());

  int idContact = contactdatabase.AddContact(strContactPath, nameValues, phoneValues, emailValues, homeValues, profValues, dateValues, relValues, IMValues, URLValues);
  
  if( idContact <=0 )
    return InternalError;
  
  // set the thumbnail for photo
  CTextureCache::Get().BackgroundCacheImage(strContactPath);
  contactdatabase.SetArtForItem(idContact, "contact", "thumb", strContactPath);
  
  //set the latest added photo as the thumbnail for the album
  CTextureCache::Get().BackgroundCacheImage(strContactPath);
  contactdatabase.SetArtForItem(idContact, "album", "thumb", strContactPath);
  
  

  return GetContacts(method, transport, client, parameterObject, result);
}


