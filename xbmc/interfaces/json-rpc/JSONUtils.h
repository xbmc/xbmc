#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <string.h>
#include <stdlib.h>

#include "JSONRPCUtils.h"
#include "XBDateTime.h"
#include "utils/SortUtils.h"
#include "playlists/SmartPlayList.h"
#include "utils/JSONVariantWriter.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"

namespace JSONRPC
{
  /*!
   \brief Possible value types of a parameter or return type
   */
  enum JSONSchemaType
  {
    NullValue = 0x01,
    StringValue = 0x02,
    NumberValue = 0x04,
    IntegerValue = 0x08,
    BooleanValue = 0x10,
    ArrayValue = 0x20,
    ObjectValue = 0x40,
    AnyValue = 0x80
  };

  /*!
   \ingroup jsonrpc
   \brief Helper class containing utility methods to handle
   json rpc method calls.*/
  class CJSONUtils
  {
  public:
    static void MillisecondsToTimeObject(int time, CVariant &result)
    {
      int ms = time % 1000;
      result["milliseconds"] = ms;
      time = (time - ms) / 1000;

      int s = time % 60;
      result["seconds"] = s;
      time = (time - s) / 60;

      int m = time % 60;
      result["minutes"] = m;
      time = (time -m) / 60;

      result["hours"] = time;
    }

  protected:
    static void HandleLimits(const CVariant &parameterObject, CVariant &result, int size, int &start, int &end)
    {
      if (size < 0)
        size = 0;
    
      start = (int)parameterObject["limits"]["start"].asInteger();
      end   = (int)parameterObject["limits"]["end"].asInteger();
      end = (end <= 0 || end > size) ? size : end;
      start = start > end ? end : start;

      result["limits"]["start"] = start;
      result["limits"]["end"]   = end;
      result["limits"]["total"] = size;
    }

    static bool ParseSorting(const CVariant &parameterObject, SortBy &sortBy, SortOrder &sortOrder, SortAttribute &sortAttributes)
    {
      std::string method = parameterObject["sort"]["method"].asString();
      std::string order = parameterObject["sort"]["order"].asString();
      StringUtils::ToLower(method);
      StringUtils::ToLower(order);

      sortAttributes = SortAttributeNone;
      if (parameterObject["sort"]["ignorearticle"].asBoolean())
        sortAttributes = SortAttributeIgnoreArticle;
      else
        sortAttributes = SortAttributeNone;

      if (order == "ascending")
        sortOrder = SortOrderAscending;
      else if (order == "descending")
        sortOrder = SortOrderDescending;
      else
        return false;

      if (method == "none")
        sortBy = SortByNone;
      else if (method == "label")
        sortBy = SortByLabel;
      else if (method == "date")
        sortBy = SortByDate;
      else if (method == "size")
        sortBy = SortBySize;
      else if (method == "file")
        sortBy = SortByFile;
      else if (method == "path")
        sortBy = SortByPath;
      else if (method == "drivetype")
        sortBy = SortByDriveType;
      else if (method == "title")
        sortBy = SortByTitle;
      else if (method == "track")
        sortBy = SortByTrackNumber;
      else if (method == "time")
        sortBy = SortByTime;
      else if (method == "artist")
        sortBy = SortByArtist;
      else if (method == "album")
        sortBy = SortByAlbum;
      else if (method == "albumtype")
        sortBy = SortByAlbumType;
      else if (method == "genre")
        sortBy = SortByGenre;
      else if (method == "country")
        sortBy = SortByCountry;
      else if (method == "year")
        sortBy = SortByYear;
      else if (method == "rating")
        sortBy = SortByRating;
      else if (method == "votes")
        sortBy = SortByVotes;
      else if (method == "top250")
        sortBy = SortByTop250;
      else if (method == "programcount")
        sortBy = SortByProgramCount;
      else if (method == "playlist")
        sortBy = SortByPlaylistOrder;
      else if (method == "episode")
        sortBy = SortByEpisodeNumber;
      else if (method == "season")
        sortBy = SortBySeason;
      else if (method == "totalepisodes")
        sortBy = SortByNumberOfEpisodes;
      else if (method == "watchedepisodes")
        sortBy = SortByNumberOfWatchedEpisodes;
      else if (method == "tvshowstatus")
        sortBy = SortByTvShowStatus;
      else if (method == "tvshowtitle")
        sortBy = SortByTvShowTitle;
      else if (method == "sorttitle")
        sortBy = SortBySortTitle;
      else if (method == "productioncode")
        sortBy = SortByProductionCode;
      else if (method == "mpaa")
        sortBy = SortByMPAA;
      else if (method == "studio")
        sortBy = SortByStudio;
      else if (method == "dateadded")
        sortBy = SortByDateAdded;
      else if (method == "lastplayed")
        sortBy = SortByLastPlayed;
      else if (method == "playcount")
        sortBy = SortByPlaycount;
      else if (method == "listeners")
        sortBy = SortByListeners;
      else if (method == "bitrate")
        sortBy = SortByBitrate;
      else if (method == "random")
        sortBy = SortByRandom;
      else
        return false;

      return true;
    }

    static void ParseLimits(const CVariant &parameterObject, int &limitStart, int &limitEnd)
    {
      limitStart = (int)parameterObject["limits"]["start"].asInteger();
      limitEnd = (int)parameterObject["limits"]["end"].asInteger();
    }
  
    /*!
     \brief Checks if the given object contains a parameter
     \param parameterObject Object to check for a parameter
     \param key Possible name of the parameter
     \param position Possible position of the parameter
     \return True if the parameter is available otherwise false

     Checks the given object for a parameter with the given key (if
     the given object is not an array) or for a parameter at the 
     given position (if the given object is an array).
     */
    static inline bool ParameterExists(const CVariant &parameterObject, std::string key, unsigned int position) { return IsValueMember(parameterObject, key) || (parameterObject.isArray() && parameterObject.size() > position); }

    /*!
     \brief Checks if the given object contains a value
     with the given key
     \param value Value to check for the member
     \param key Key of the member to check for
     \return True if the given object contains a member with 
     the given key otherwise false
     */
    static inline bool IsValueMember(const CVariant &value, std::string key) { return value.isObject() && value.isMember(key); }
    
    /*!
     \brief Returns the json value of a parameter
     \param parameterObject Object containing all provided parameters
     \param key Possible name of the parameter
     \param position Possible position of the parameter
     \return Json value of the parameter with the given name or at the
     given position

     Returns the value of the parameter with the given key (if
     the given object is not an array) or of the parameter at the 
     given position (if the given object is an array).
     */
    static inline CVariant GetParameter(const CVariant &parameterObject, std::string key, unsigned int position) { return IsValueMember(parameterObject, key) ? parameterObject[key] : parameterObject[position]; }
    
    /*!
     \brief Returns the json value of a parameter or the given
     default value
     \param parameterObject Object containing all provided parameters
     \param key Possible name of the parameter
     \param position Possible position of the parameter
     \param fallback Default value of the parameter
     \return Json value of the parameter with the given name or at the
     given position or the default value if the parameter does not exist

     Returns the value of the parameter with the given key (if
     the given object is not an array) or of the parameter at the 
     given position (if the given object is an array). If the
     parameter does not exist the given default value is returned.
     */
    static inline CVariant GetParameter(const CVariant &parameterObject, std::string key, unsigned int position, CVariant fallback) { return IsValueMember(parameterObject, key) ? parameterObject[key] : ((parameterObject.isArray() && parameterObject.size() > position) ? parameterObject[position] : fallback); }
    
    /*!
     \brief Returns the given json value as a string
     \param value Json value to convert to a string
     \param defaultValue Default string value
     \return String value of the given json value or the default value
     if the given json value is no string
     */
    static inline std::string GetString(const CVariant &value, const char* defaultValue)
    {
      std::string str = defaultValue;
      if (value.isString())
      {
        str = value.asString();
      }

      return str;
    }

    /*!
     \brief Returns a TransportLayerCapability value of the
     given string representation
     \param transport String representation of the TransportLayerCapability
     \return TransportLayerCapability value of the given string representation
     */
    static inline TransportLayerCapability StringToTransportLayer(std::string transport)
    {
      if (transport.compare("Announcing") == 0)
        return Announcing;
      if (transport.compare("FileDownloadDirect") == 0)
        return FileDownloadDirect;
      if (transport.compare("FileDownloadRedirect") == 0)
        return FileDownloadRedirect;

      return Response;
    }

    /*!
     \brief Returns a JSONSchemaType value for the given
     string representation
     \param valueType String representation of the JSONSchemaType
     \return JSONSchemaType value of the given string representation
     */
    static inline JSONSchemaType StringToSchemaValueType(std::string valueType)
    {
      if (valueType.compare("null") == 0)
        return NullValue;
      if (valueType.compare("string") == 0)
        return StringValue;
      if (valueType.compare("number") == 0)
        return NumberValue;
      if (valueType.compare("integer") == 0)
        return IntegerValue;
      if (valueType.compare("boolean") == 0)
        return BooleanValue;
      if (valueType.compare("array") == 0)
        return ArrayValue;
      if (valueType.compare("object") == 0)
        return ObjectValue;

      return AnyValue;
    }
    
    /*!
     \brief Returns a string representation for the 
     given JSONSchemaType
     \param valueType Specific JSONSchemaType
     \return String representation of the given JSONSchemaType
     */
    static inline std::string SchemaValueTypeToString(JSONSchemaType valueType)
    {
      std::vector<JSONSchemaType> types = std::vector<JSONSchemaType>();
      for (unsigned int value = 0x01; value <= (unsigned int)AnyValue; value *= 2)
      {
        if (HasType(valueType, (JSONSchemaType)value))
          types.push_back((JSONSchemaType)value);
      }

      std::string strType;
      if (types.size() > 1)
        strType.append("[");

      for (unsigned int index = 0; index < types.size(); index++)
      {
        if (index > 0)
          strType.append(", ");

        switch (types.at(index))
        {
        case StringValue:
          strType.append("string");
          break;
        case NumberValue:
          strType.append("number");
          break;
        case IntegerValue:
          strType.append("integer");
          break;
        case BooleanValue:
          strType.append("boolean");
          break;
        case ArrayValue:
          strType.append("array");
          break;
        case ObjectValue:
          strType.append("object");
          break;
        case AnyValue:
          strType.append("any");
          break;
        case NullValue:
          strType.append("null");
          break;
        default:
          strType.append("unknown");
        }
      }

      if (types.size() > 1)
        strType.append("]");

      return strType;
    }

    /*!
     \brief Converts the given json schema type into
     a json object
     \param valueTye json schema type(s)
     \param jsonObject json object into which the json schema type(s) are stored
     */
    static inline void SchemaValueTypeToJson(JSONSchemaType valueType, CVariant &jsonObject)
    {
      jsonObject = CVariant(CVariant::VariantTypeArray);
      for (unsigned int value = 0x01; value <= (unsigned int)AnyValue; value *= 2)
      {
        if (HasType(valueType, (JSONSchemaType)value))
          jsonObject.append(SchemaValueTypeToString((JSONSchemaType)value));
      }

      if (jsonObject.size() == 1)
      {
        CVariant jsonType = jsonObject[0];
        jsonObject = jsonType;
      }
    }

    static inline const char *ValueTypeToString(CVariant::VariantType valueType)
    {
      switch (valueType)
      {
      case CVariant::VariantTypeString:
        return "string";
      case CVariant::VariantTypeDouble:
        return "number";
      case CVariant::VariantTypeInteger:
      case CVariant::VariantTypeUnsignedInteger:
        return "integer";
      case CVariant::VariantTypeBoolean:
        return "boolean";
      case CVariant::VariantTypeArray:
        return "array";
      case CVariant::VariantTypeObject:
        return "object";
      case CVariant::VariantTypeNull:
      case CVariant::VariantTypeConstNull:
        return "null";
      default:
        return "unknown";
      }
    }

    /*!
     \brief Checks if the parameter with the given name or at
     the given position is of a certain type
     \param parameterObject Object containing all provided parameters
     \param key Possible name of the parameter
     \param position Possible position of the parameter
     \param valueType Expected type of the parameter
     \return True if the specific parameter is of the given type otherwise false
     */
    static inline bool IsParameterType(const CVariant &parameterObject, const char *key, unsigned int position, JSONSchemaType valueType)
    {
      if ((valueType & AnyValue) == AnyValue)
        return true;

      CVariant parameter;
      if (IsValueMember(parameterObject, key))
        parameter = parameterObject[key];
      else if(parameterObject.isArray() && parameterObject.size() > position)
        parameter = parameterObject[position];

      return IsType(parameter, valueType);
    }

    /*!
     \brief Checks if the given json value is of the given type
     \param value Json value to check
     \param valueType Expected type of the json value
     \return True if the given json value is of the given type otherwise false
    */
    static inline bool IsType(const CVariant &value, JSONSchemaType valueType)
    {
      if (HasType(valueType, AnyValue))
        return true;
      if (HasType(valueType, StringValue) && value.isString())
        return true;
      if (HasType(valueType, NumberValue) && (value.isInteger() || value.isUnsignedInteger() || value.isDouble()))
        return true;
      if (HasType(valueType, IntegerValue) && (value.isInteger() || value.isUnsignedInteger()))
        return true;
      if (HasType(valueType, BooleanValue) && value.isBoolean())
        return true;
      if (HasType(valueType, ArrayValue) && value.isArray())
        return true;
      if (HasType(valueType, ObjectValue) && value.isObject())
        return true;

      return value.isNull();
    }

    /*!
     \brief Sets the value of the given json value to the
     default value of the given type
     \param value Json value to be set
     \param valueType Type of the default value
     */
    static inline void SetDefaultValue(CVariant &value, JSONSchemaType valueType)
    {
      switch (valueType)
      {
        case StringValue:
          value = CVariant("");
          break;
        case NumberValue:
          value = CVariant(CVariant::VariantTypeDouble);
          break;
        case IntegerValue:
          value = CVariant(CVariant::VariantTypeInteger);
          break;
        case BooleanValue:
          value = CVariant(CVariant::VariantTypeBoolean);
          break;
        case ArrayValue:
          value = CVariant(CVariant::VariantTypeArray);
          break;
        case ObjectValue:
          value = CVariant(CVariant::VariantTypeObject);
          break;
        default:
          value = CVariant(CVariant::VariantTypeNull);
      }
    }

    static inline bool HasType(JSONSchemaType typeObject, JSONSchemaType type) { return (typeObject & type) == type; }

    static inline bool ParameterNotNull(const CVariant &parameterObject, std::string key) { return parameterObject.isMember(key) && !parameterObject[key].isNull(); }

    /*!
     \brief Copies the values from the jsonStringArray to the stringArray.
     stringArray is cleared.
     \param jsonStringArray JSON object representing a string array
     \param stringArray String array where the values are copied into (cleared)
     */
    static void CopyStringArray(const CVariant &jsonStringArray, std::vector<std::string> &stringArray)
    {
      if (!jsonStringArray.isArray())
        return;

      stringArray.clear();
      for (CVariant::const_iterator_array it = jsonStringArray.begin_array(); it != jsonStringArray.end_array(); ++it)
        stringArray.push_back(it->asString());
    }

    static void SetFromDBDate(const CVariant &jsonDate, CDateTime &date)
    {
      if (!jsonDate.isString())
        return;

      if (jsonDate.empty())
        date.Reset();
      else
        date.SetFromDBDate(jsonDate.asString());
    }

    static void SetFromDBDateTime(const CVariant &jsonDate, CDateTime &date)
    {
      if (!jsonDate.isString())
        return;

      if (jsonDate.empty())
        date.Reset();
      else
        date.SetFromDBDateTime(jsonDate.asString());
    }

    static bool GetXspFiltering(const std::string &type, const CVariant &filter, std::string &xsp)
    {
      if (type.empty() || !filter.isObject())
        return false;

      CVariant xspObj(CVariant::VariantTypeObject);
      xspObj["type"] = type;

      if (filter.isMember("field"))
      {
        xspObj["rules"]["and"] = CVariant(CVariant::VariantTypeArray);
        xspObj["rules"]["and"].push_back(filter);
      }
      else
        xspObj["rules"] = filter;

      CSmartPlaylist playlist;
      return playlist.Load(xspObj) && playlist.SaveAsJson(xsp, false);
    }
  };
}
