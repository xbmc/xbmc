#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string.h>
#include <stdlib.h>
#include "utils/StdString.h"
#include "interfaces/IAnnouncer.h"
#include "interfaces/AnnouncementUtils.h"
#include "ITransportLayer.h"
#include "jsoncpp/include/json/json.h"
#include "utils/Variant.h"

using namespace ANNOUNCEMENT;

namespace JSONRPC
{
  /*!
   \ingroup jsonrpc
   \brief Possible statuc codes of a response
   to a JSON RPC request
   */
  enum JSON_STATUS
  {
    OK = 0,
    ACK = -1,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    ParseError = -32700,
    //-32099..-32000 Reserved for implementation-defined server-errors.
    BadPermission = -32099,
    FailedToExecute = -32100
  };

  /*!
   \brief Function pointer for json rpc methods
   */
  typedef JSON_STATUS (*MethodCall) (const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);

  /*!
   \ingroup jsonrpc
   \brief Permission categories for json rpc methods
   
   A json rpc method will only be called if the caller 
   has the correct permissions to exectue the method.
   The method call needs to be perfectly threadsafe.
  */
  enum OperationPermission
  {
    ReadData = 0x1,
    ControlPlayback = 0x2,
    ControlNotify = 0x4,
    ControlPower = 0x8,
    Logging = 0x10,
    ScanLibrary = 0x20,
    Navigate = 0x40
  };

  static const int OPERATION_PERMISSION_ALL = (ReadData | ControlPlayback | ControlNotify | ControlPower | Logging | ScanLibrary | Navigate);

  static const int OPERATION_PERMISSION_NOTIFICATION = (ControlPlayback | ControlNotify | ControlPower | Logging | ScanLibrary | Navigate);

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
  protected:
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
    static inline bool ParameterExists(const Json::Value &parameterObject, std::string key, unsigned int position) { return IsValueMember(parameterObject, key) || (parameterObject.isArray() && parameterObject.size() > position); }

    /*!
     \brief Checks if the given object contains a value
     with the given key
     \param value Value to check for the member
     \param key Key of the member to check for
     \return True if the given object contains a member with 
     the given key otherwise false
     */
    static inline bool IsValueMember(const Json::Value &value, std::string key) { return value.isObject() && value.isMember(key); }
    
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
    static inline Json::Value GetParameter(const Json::Value &parameterObject, std::string key, unsigned int position) { return IsValueMember(parameterObject, key) ? parameterObject[key] : parameterObject[position]; }
    
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
    static inline Json::Value GetParameter(const Json::Value &parameterObject, std::string key, unsigned int position, Json::Value fallback) { return IsValueMember(parameterObject, key) ? parameterObject[key] : ((parameterObject.isArray() && parameterObject.size() > position) ? parameterObject[position] : fallback); }
    
    /*!
     \brief Returns the given json value as a string
     \param value Json value to convert to a string
     \param defaultValue Default string value
     \return String value of the given json value or the default value
     if the given json value is no string
     */
    static inline std::string GetString(const Json::Value &value, const char* defaultValue)
    {
      std::string str = defaultValue;
      if (value.isString())
      {
        str = value.asString();
      }

      return str;
    }

    /*!
     \brief Returns a string representation for the 
     given OperationPermission
     \param permission Specific OperationPermission
     \return String representation of the given OperationPermission
     */
    static inline const char *PermissionToString(const OperationPermission &permission)
    {
      switch (permission)
      {
      case ReadData:
        return "ReadData";
      case ControlPlayback:
        return "ControlPlayback";
      case ControlNotify:
        return "ControlNotify";
      case ControlPower:
        return "ControlPower";
      case Logging:
        return "Logging";
      case ScanLibrary:
        return "ScanLibrary";
      case Navigate:
        return "Navigate";
      default:
        return "Unknown";
      }
    }

    /*!
     \brief Returns a OperationPermission value for the given
     string representation
     \param permission String representation of the OperationPermission
     \return OperationPermission value of the given string representation
     */
    static inline OperationPermission StringToPermission(std::string permission)
    {
      if (permission.compare("ControlPlayback") == 0)
        return ControlPlayback;
      if (permission.compare("ControlNotify") == 0)
        return ControlNotify;
      if (permission.compare("ControlPower") == 0)
        return ControlPower;
      if (permission.compare("Logging") == 0)
        return Logging;
      if (permission.compare("ScanLibrary") == 0)
        return ScanLibrary;
      if (permission.compare("Navigate") == 0)
        return Navigate;

      return ReadData;
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
      if (transport.compare("FileDownload") == 0)
        return FileDownload;

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
    static inline void SchemaValueTypeToJson(JSONSchemaType valueType, Json::Value &jsonObject)
    {
      jsonObject = Json::Value(Json::arrayValue);
      for (unsigned int value = 0x01; value <= (unsigned int)AnyValue; value *= 2)
      {
        if (HasType(valueType, (JSONSchemaType)value))
          jsonObject.append(SchemaValueTypeToString((JSONSchemaType)value));
      }

      if (jsonObject.size() == 1)
        jsonObject = jsonObject[0];
    }

    static inline const char *ValueTypeToString(Json::ValueType valueType)
    {
      switch (valueType)
      {
      case Json::stringValue:
        return "string";
      case Json::realValue:
        return "number";
      case Json::intValue:
      case Json::uintValue:
        return "integer";
      case Json::booleanValue:
        return "boolean";
      case Json::arrayValue:
        return "array";
      case Json::objectValue:
        return "object";
      case Json::nullValue:
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
    static inline bool IsParameterType(const Json::Value &parameterObject, const char *key, unsigned int position, JSONSchemaType valueType)
    {
      if ((valueType & AnyValue) == AnyValue)
        return true;

      Json::Value parameter;
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
    static inline bool IsType(const Json::Value &value, JSONSchemaType valueType)
    {
      if (HasType(valueType, AnyValue))
        return true;
      if (HasType(valueType, StringValue) && value.isString())
        return true;
      if (HasType(valueType, NumberValue) && (value.isInt() || value.isUInt() || value.isDouble()))
        return true;
      if (HasType(valueType, IntegerValue) && (value.isInt() || value.isUInt()))
        return true;
      if (HasType(valueType, BooleanValue) && value.isBool())
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
    static inline void SetDefaultValue(Json::Value &value, JSONSchemaType valueType)
    {
      switch (valueType)
      {
        case StringValue:
          value = Json::Value("");
          break;
        case NumberValue:
          value = Json::Value(Json::realValue);
          break;
        case IntegerValue:
          value = Json::Value(Json::intValue);
          break;
        case BooleanValue:
          value = Json::Value(Json::booleanValue);
          break;
        case ArrayValue:
          value = Json::Value(Json::arrayValue);
          break;
        case ObjectValue:
          value = Json::Value(Json::objectValue);
          break;
        default:
          value = Json::Value(Json::nullValue);
      }
    }

    static inline bool HasType(JSONSchemaType typeObject, JSONSchemaType type) { return (typeObject & type) == type; }

    static inline int Compare(const Json::Value &value, const Json::Value &other)
    {
      if (value.type() != other.type())
        return other.type() - value.type();

      int result = 0;
      Json::Value::Members members, otherMembers;

      switch (value.type())
      {
      case Json::nullValue:
        return 0;
      case Json::intValue:
        return other.asInt() - value.asInt();
      case Json::uintValue:
        return other.asUInt() - value.asUInt();
      case Json::realValue:
        return (int)(other.asDouble() - value.asDouble());
      case Json::booleanValue:
        return other.asBool() - value.asBool();
      case Json::stringValue:
        return other.asString().compare(value.asString());
      case Json::arrayValue:
        if (other.size() != value.size())
          return other.size() - value.size();

        for (unsigned int i = 0; i < other.size(); i++)
        {
          if ((result = Compare(value[i], other[i])) != 0)
            return result;
        }

        return 0;
      case Json::objectValue:
        members = value.getMemberNames();
        otherMembers = other.getMemberNames();

        if (members.size() != otherMembers.size())
          return otherMembers.size() - members.size();

        for (unsigned int i = 0; i < members.size(); i++)
        {
          if (!other.isMember(members.at(i)))
            return -1;

          if ((result = Compare(value[members.at(i)], other[members.at(i)])) != 0)
            return result;
        }

        return 0;
      }

      return -1;  // unreachable
    }

    static std::string AnnouncementToJSON(EAnnouncementFlag flag, const char *sender, const char *method, const CVariant &data, bool compactOutput)
    {
      Json::Value root;
      root["jsonrpc"] = "2.0";

      CStdString namespaceMethod;
      namespaceMethod.Format("%s.%s", CAnnouncementUtils::AnnouncementFlagToString(flag), method);
      root["method"]  = namespaceMethod.c_str();

      if (data.isObject())
        data.toJsonValue(root["params"]);
      root["params"]["sender"] = sender;

      Json::Writer *writer;
      if (compactOutput)
        writer = new Json::FastWriter();
      else
        writer = new Json::StyledWriter();

      std::string str = writer->write(root);
      delete writer;
      return str;
    }
  };
}
