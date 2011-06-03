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

#include <limits>
#include "ServiceDescription.h"
#include "JSONServiceDescription.h"
#include "utils/log.h"
#include "utils/StdString.h"
#include "utils/JSONVariantParser.h"

using namespace std;
using namespace JSONRPC;

JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::CJsonSchemaPropertiesMap()
{
  m_propertiesmap = std::map<std::string, JSONSchemaTypeDefinition>();
}

void JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::add(JSONSchemaTypeDefinition &property)
{
  CStdString name = property.name;
  name = name.ToLower();
  m_propertiesmap[name] = property;
}

JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::begin() const
{
  return m_propertiesmap.begin();
}

JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::find(const std::string& key) const
{
  return m_propertiesmap.find(key);
}

JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::end() const
{
  return m_propertiesmap.end();
}

unsigned int JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::size() const
{
  return m_propertiesmap.size();
}

CVariant CJSONServiceDescription::m_notifications = CVariant(CVariant::VariantTypeObject);
CJSONServiceDescription::CJsonRpcMethodMap CJSONServiceDescription::m_actionMap;
JsonRpcDescriptionHeader CJSONServiceDescription::m_header;
std::map<std::string, JSONSchemaTypeDefinition> CJSONServiceDescription::m_types = std::map<std::string, JSONSchemaTypeDefinition>();
std::vector<JsonRpcMethodMap> CJSONServiceDescription::m_unresolvedMethods = std::vector<JsonRpcMethodMap>();
bool CJSONServiceDescription::m_newReferenceType = false;

bool CJSONServiceDescription::Parse(JsonRpcMethodMap methodMap[], unsigned int size)
{
  // Read the json schema for notifications
  m_notifications = CJSONVariantParser::Parse((const unsigned char *)JSON_NOTIFICATION_DESCRIPTION, strlen(JSON_NOTIFICATION_DESCRIPTION));
  if (m_notifications.isNull())
    CLog::Log(LOGERROR, "JSONRPC: Unable to read the json schema notification description");

  // Read the json schema service descriptor and check if it represents
  // a json object and contains a "services" element for methods
  CVariant descriptionObject = CJSONVariantParser::Parse((const unsigned char *)JSON_SERVICE_DESCRIPTION, strlen(JSON_SERVICE_DESCRIPTION));
  if (descriptionObject.isNull())
  {
    CLog::Log(LOGERROR, "JSONRPC: Unable to read the json schema service description");
    return false;
  }

  // First parse the header
  parseHeader(descriptionObject);

  // At the beginning all methods are unresolved
  for (unsigned int index = 0; index < size; index++)
  {
    m_unresolvedMethods.push_back(methodMap[index]);
  }

  // As long as there have been new reference types
  // and there are more bad methods than in the last
  // try we can try parsing again
  unsigned int unresolvedMethodCount = m_unresolvedMethods.size() + 1;
  m_newReferenceType = true;
  while (m_newReferenceType && m_unresolvedMethods.size() > 0 && m_unresolvedMethods.size() < unresolvedMethodCount)
  {
    m_newReferenceType = false;
    unresolvedMethodCount = m_unresolvedMethods.size();
    std::vector<JsonRpcMethodMap> stillUnresolvedMethods = std::vector<JsonRpcMethodMap>();

    // Loop through the methods
    std::vector<JsonRpcMethodMap>::const_iterator iteratorEnd = m_unresolvedMethods.end();
    for (std::vector<JsonRpcMethodMap>::const_iterator iterator = m_unresolvedMethods.begin(); iterator != iteratorEnd; iterator++)
    {
      // Make sure the method description actually exists and represents an object
      if (!descriptionObject.isMember((*iterator).name) || !descriptionObject[(*iterator).name].isObject() || 
          !descriptionObject[(*iterator).name].isMember("type") || !descriptionObject[(*iterator).name]["type"].isString())
      {
          CLog::Log(LOGERROR, "JSONRPC: No json schema description for method %s found", (*iterator).name.c_str());
          continue;
      }

      std::string type = GetString(descriptionObject[(*iterator).name]["type"], "");
      if (type.compare("method") != 0)
      {
        CLog::Log(LOGERROR, "JSONRPC: No valid json schema description for method %s found", (*iterator).name.c_str());
          continue;
      }

      // Parse the details of the method
      JsonRpcMethod method;
      method.name = (*iterator).name;
      method.method = (*iterator).method;
      if (!parseMethod(descriptionObject[method.name], method))
      {
        // If parsing failed add the method to the list of currently bad methods
        // (might be that a reference for a parameter is missing)
        stillUnresolvedMethods.push_back((*iterator));
        CLog::Log(LOGDEBUG, "JSONRPC: Method %s could not be parsed correctly and might be re-parsed later", method.name.c_str());
        continue;
      }

      m_actionMap.add(method);
    }

    m_unresolvedMethods = stillUnresolvedMethods;
  }

  // Parse
  for (CVariant::const_iterator_map itr = descriptionObject.begin_map(); itr != descriptionObject.end_map(); itr++)
  {
    // Make sure the method actually exists and represents an object
    if (!itr->second.isObject() || !itr->second.isMember("type") || !itr->second["type"].isString())
      continue;

    std::string type = GetString(itr->second["type"], "");
    if (type.compare("method") != 0 && itr->second.isMember("id") && itr->second["id"].isString())
    {
      JSONSchemaTypeDefinition globalType;
      globalType.name = itr->first;
      parseTypeDefinition(itr->second, globalType, false);
    }
  }

  // Print a log message for every unparseable method
  std::vector<JsonRpcMethodMap>::const_iterator iteratorEnd = m_unresolvedMethods.end();
  for (std::vector<JsonRpcMethodMap>::const_iterator iterator = m_unresolvedMethods.begin(); iterator != iteratorEnd; iterator++)
    CLog::Log(LOGERROR, "JSONRPC: Method %s could not be parsed correctly and will be ignored", (*iterator).name.c_str());

  return true;
}

int CJSONServiceDescription::GetVersion()
{
  return m_header.version;
}

void CJSONServiceDescription::Print(CVariant &result, ITransportLayer *transport, IClient *client, bool printDescriptions, bool printMetadata, bool filterByTransport)
{
  // Print the header
  result["id"] = m_header.ID;
  result["version"] = m_header.version;
  result["description"] = m_header.description;

  std::map<std::string, JSONSchemaTypeDefinition>::const_iterator typeIterator;
  std::map<std::string, JSONSchemaTypeDefinition>::const_iterator typeIteratorEnd = m_types.end();
  for (typeIterator = m_types.begin(); typeIterator != typeIteratorEnd; typeIterator++)
  {
    CVariant currentType = CVariant(CVariant::VariantTypeObject);
    printType(typeIterator->second, false, true, true, printDescriptions, currentType);

    result["types"][typeIterator->first] = currentType;
  }

  // Iterate through all json rpc methods
  int clientPermissions = client->GetPermissionFlags();
  int transportCapabilities = transport->GetCapabilities();

  CJsonRpcMethodMap::JsonRpcMethodIterator methodIterator;
  CJsonRpcMethodMap::JsonRpcMethodIterator methodIteratorEnd = m_actionMap.end();
  for (methodIterator = m_actionMap.begin(); methodIterator != methodIteratorEnd; methodIterator++)
  {
    if ((clientPermissions & methodIterator->second.permission) == 0 || ((transportCapabilities & methodIterator->second.transportneed) == 0 && filterByTransport))
      continue;

    CVariant currentMethod = CVariant(CVariant::VariantTypeObject);

    currentMethod["type"] = "method";
    if (printDescriptions && !methodIterator->second.description.empty())
      currentMethod["description"] = methodIterator->second.description;
    if (printMetadata)
    {
      currentMethod["permission"] = PermissionToString(methodIterator->second.permission);
    }

    currentMethod["params"] = CVariant(CVariant::VariantTypeArray);
    for (unsigned int paramIndex = 0; paramIndex < methodIterator->second.parameters.size(); paramIndex++)
    {
      CVariant param = CVariant(CVariant::VariantTypeObject);
      printType(methodIterator->second.parameters.at(paramIndex), true, false, true, printDescriptions, param);
      currentMethod["params"].append(param);
    }

    currentMethod["returns"] = methodIterator->second.returns;

    result["methods"][methodIterator->second.name] = currentMethod;
  }

  // Print notification description
  for (CVariant::const_iterator_map itr = m_notifications.begin_map(); itr != m_notifications.end_map(); itr++)
  {
    if (!itr->second.isObject() ||
        !itr->second.isMember("type") ||
        !itr->second["type"].isString() ||
         itr->second["type"] == CVariant("notification"))
    {
      continue;
    }

    result["notifications"][itr->first] = itr->second;
  }
}

JSON_STATUS CJSONServiceDescription::CheckCall(const char* const method, const CVariant &requestParameters, IClient *client, bool notification, MethodCall &methodCall, CVariant &outputParameters)
{
  CJsonRpcMethodMap::JsonRpcMethodIterator iter = m_actionMap.find(method);
  if (iter != m_actionMap.end())
  {
    if (client != NULL && (client->GetPermissionFlags() & iter->second.permission) && (!notification || (iter->second.permission & OPERATION_PERMISSION_NOTIFICATION)))
    {
      methodCall = iter->second.method;

      // Count the number of actually handled (present)
      // parameters
      unsigned int handled = 0;
      CVariant errorData = CVariant(CVariant::VariantTypeObject);
      errorData["method"] = iter->second.name;

      // Loop through all the parameters to check
      for (unsigned int i = 0; i < iter->second.parameters.size(); i++)
      {
        // Evaluate the current parameter
        JSON_STATUS status = checkParameter(requestParameters, iter->second.parameters.at(i), i, outputParameters, handled, errorData);
        if (status != OK)
        {
          // Return the error data object in the outputParameters reference
          outputParameters = errorData;
          return status;
        }
      }

      // Check if there were unnecessary parameters
      if (handled < requestParameters.size())
      {
        errorData["message"] = "Too many parameters";
        outputParameters = errorData;
        return InvalidParams;
      }

      return OK;
    }
    else
      return BadPermission;
  }
  else
    return MethodNotFound;
}

void CJSONServiceDescription::printType(const JSONSchemaTypeDefinition &type, bool isParameter, bool isGlobal, bool printDefault, bool printDescriptions, CVariant &output)
{
  bool typeReference = false;

  // Printing general fields
  if (isParameter)
    output["name"] = type.name;

  if (isGlobal)
    output["id"] = type.ID;
  else if (!type.ID.empty())
  {
    output["$ref"] = type.ID;
    typeReference = true;
  }

  if (printDescriptions && !type.description.empty())
    output["description"] = type.description;

  if (isParameter || printDefault)
  {
    if (!type.optional)
      output["required"] = true;
    if (type.optional && type.type != ObjectValue && type.type != ArrayValue)
      output["default"] = type.defaultValue;
  }

  if (!typeReference)
  {
    SchemaValueTypeToJson(type.type, output["type"]);

    // Printing enum field
    if (type.enums.size() > 0)
    {
      output["enums"] = CVariant(CVariant::VariantTypeArray);
      for (unsigned int enumIndex = 0; enumIndex < type.enums.size(); enumIndex++)
        output["enums"].append(type.enums.at(enumIndex));
    }

    // Printing integer/number fields
    if (HasType(type.type, IntegerValue) || HasType(type.type, NumberValue))
    {
      if (HasType(type.type, NumberValue))
      {
        if (type.minimum > -numeric_limits<double>::max())
          output["minimum"] = type.minimum;
        if (type.maximum < numeric_limits<double>::max())
          output["maximum"] = type.maximum;
      }
      else
      {
        if (type.minimum > numeric_limits<int>::min())
          output["minimum"] = (int)type.minimum;
        if (type.maximum < numeric_limits<int>::max())
          output["maximum"] = (int)type.maximum;
      }

      if (type.exclusiveMinimum)
        output["exclusiveMinimum"] = true;
      if (type.exclusiveMaximum)
        output["exclusiveMaximum"] = true;
      if (type.divisibleBy > 0)
        output["divisibleBy"] = type.divisibleBy;
    }

    // Print array fields
    if (HasType(type.type, ArrayValue))
    {
      if (type.items.size() == 1)
      {
        printType(type.items.at(0), false, false, false, printDescriptions, output["items"]);
      }
      else if (type.items.size() > 1)
      {
        output["items"] = CVariant(CVariant::VariantTypeArray);
        for (unsigned int itemIndex = 0; itemIndex < type.items.size(); itemIndex++)
        {
          CVariant item = CVariant(CVariant::VariantTypeObject);
          printType(type.items.at(itemIndex), false, false, false, printDescriptions, item);
          output["items"].append(item);
        }
      }

      if (type.minItems > 0)
        output["minItems"] = type.minItems;
      if (type.maxItems > 0)
        output["maxItems"] = type.maxItems;

      if (type.additionalItems.size() == 1)
      {
        printType(type.additionalItems.at(0), false, false, false, printDescriptions, output["additionalItems"]);
      }
      else if (type.additionalItems.size() > 1)
      {
        output["additionalItems"] = CVariant(CVariant::VariantTypeArray);
        for (unsigned int addItemIndex = 0; addItemIndex < type.additionalItems.size(); addItemIndex++)
        {
          CVariant item = CVariant(CVariant::VariantTypeObject);
          printType(type.additionalItems.at(addItemIndex), false, false, false, printDescriptions, item);
          output["additionalItems"].append(item);
        }
      }

      if (type.uniqueItems)
        output["uniqueItems"] = true;
    }

    // Print object fields
    if (HasType(type.type, ObjectValue) && type.properties.size() > 0)
    {
      output["properties"] = CVariant(CVariant::VariantTypeObject);

      JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator propertiesEnd = type.properties.end();
      JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator propertiesIterator;
      for (propertiesIterator = type.properties.begin(); propertiesIterator != propertiesEnd; propertiesIterator++)
      {
        printType(propertiesIterator->second, false, false, true, printDescriptions, output["properties"][propertiesIterator->first]);
      }
    }
  }
}

JSON_STATUS CJSONServiceDescription::checkParameter(const CVariant &requestParameters, const JSONSchemaTypeDefinition &type, unsigned int position, CVariant &outputParameters, unsigned int &handled, CVariant &errorData)
{
  // Let's check if the parameter has been provided
  if (ParameterExists(requestParameters, type.name, position))
  {
    // Get the parameter
    CVariant parameterValue = GetParameter(requestParameters, type.name, position);

    // Evaluate the type of the parameter
    JSON_STATUS status = checkType(parameterValue, type, outputParameters[type.name], errorData["stack"]);
    if (status != OK)
      return status;

    // The parameter was present and valid
    handled++;
  }
  // If the parameter has not been provided but is optional
  // we can use its default value
  else if (type.optional)
    outputParameters[type.name] = type.defaultValue;
  // The parameter is required but has not been provided => invalid
  else
  {
    errorData["stack"]["name"] = type.name;
    SchemaValueTypeToJson(type.type, errorData["stack"]["type"]);
    errorData["stack"]["message"] = "Missing parameter";
    return InvalidParams;
  }

  return OK;
}

JSON_STATUS CJSONServiceDescription::checkType(const CVariant &value, const JSONSchemaTypeDefinition &type, CVariant &outputValue, CVariant &errorData)
{
  if (!type.name.empty())
    errorData["name"] = type.name;
  SchemaValueTypeToJson(type.type, errorData["type"]);
  CStdString errorMessage;

  // Let's check the type of the provided parameter
  if (!IsType(value, type.type))
  {
    CLog::Log(LOGWARNING, "JSONRPC: Type mismatch in type %s", type.name.c_str());
    errorMessage.Format("Invalid type %s received", ValueTypeToString(value.type()));
    errorData["message"] = errorMessage.c_str();
    return InvalidParams;
  }
  else if (value.isNull() && !HasType(type.type, NullValue))
  {
    CLog::Log(LOGWARNING, "JSONRPC: Value is NULL in type %s", type.name.c_str());
    errorData["message"] = "Received value is null";
    return InvalidParams;
  }

  // If it is an array we need to
  // - check the type of every element ("items")
  // - check if they need to be unique ("uniqueItems")
  if (HasType(type.type, ArrayValue) && value.isArray())
  {
    // Check the number of items against minItems and maxItems
    if ((type.minItems > 0 && value.size() < type.minItems) || (type.maxItems > 0 && value.size() > type.maxItems))
    {
      CLog::Log(LOGWARNING, "JSONRPC: Number of array elements does not match minItems and/or maxItems in type %s", type.name.c_str());
      if (type.minItems > 0 && type.maxItems > 0)
        errorMessage.Format("Between %d and %d array items expected but %d received", type.minItems, type.maxItems, value.size());
      else if (type.minItems > 0)
        errorMessage.Format("At least %d array items expected but only %d received", type.minItems, value.size());
      else
        errorMessage.Format("Only %d array items expected but %d received", type.maxItems, value.size());
      errorData["message"] = errorMessage.c_str();
      return InvalidParams;
    }

    if (type.items.size() == 0)
    {
      outputValue = value;
    }
    else if (type.items.size() == 1)
    {
      JSONSchemaTypeDefinition itemType = type.items.at(0);

      // Loop through all array elements
      for (unsigned int arrayIndex = 0; arrayIndex < value.size(); arrayIndex++)
      {
        JSON_STATUS status = checkType(value[arrayIndex], itemType, outputValue[arrayIndex], errorData["property"]);
        if (status != OK)
        {
          CLog::Log(LOGWARNING, "JSONRPC: Array element at index %u does not match in type %s", arrayIndex, type.name.c_str());
          errorMessage.Format("array element at index %u does not match", arrayIndex);
          errorData["message"] = errorMessage.c_str();
          return status;
        }
      }
    }
    // We have more than one element in "items"
    // so we have tuple typing, which means that
    // every element in the value array must match
    // with the type at the same position in the
    // "items" array
    else
    {
      // If the number of elements in the value array
      // does not match the number of elements in the
      // "items" array and additional items are not
      // allowed there is no need to check every element
      if (value.size() < type.items.size() || (value.size() != type.items.size() && type.additionalItems.size() == 0))
      {
        CLog::Log(LOGWARNING, "JSONRPC: One of the array elements does not match in type %s", type.name.c_str());
        errorMessage.Format("%d array elements expected but %d received", type.items.size(), value.size());
        errorData["message"] = errorMessage.c_str();
        return InvalidParams;
      }

      // Loop through all array elements until there
      // are either no more schemas in the "items"
      // array or no more elements in the value's array
      unsigned int arrayIndex;
      for (arrayIndex = 0; arrayIndex < min(type.items.size(), (size_t)value.size()); arrayIndex++)
      {
        JSON_STATUS status = checkType(value[arrayIndex], type.items.at(arrayIndex), outputValue[arrayIndex], errorData["property"]);
        if (status != OK)
        {
          CLog::Log(LOGWARNING, "JSONRPC: Array element at index %u does not match with items schema in type %s", arrayIndex, type.name.c_str());
          return status;
        }
      }

      if (type.additionalItems.size() > 0)
      {
        // Loop through the rest of the elements
        // in the array and check them against the
        // "additionalItems"
        for (; arrayIndex < value.size(); arrayIndex++)
        {
          bool ok = false;
          for (unsigned int additionalIndex = 0; additionalIndex < type.additionalItems.size(); additionalIndex++)
          {
            CVariant dummyError;
            if (checkType(value[arrayIndex], type.additionalItems.at(additionalIndex), outputValue[arrayIndex], dummyError) == OK)
            {
              ok = true;
              break;
            }
          }

          if (!ok)
          {
            CLog::Log(LOGWARNING, "JSONRPC: Array contains non-conforming additional items in type %s", type.name.c_str());
            errorMessage.Format("Array element at index %u does not match the \"additionalItems\" schema", arrayIndex);
            errorData["message"] = errorMessage.c_str();
            return InvalidParams;
          }
        }
      }
    }

    // If every array element is unique we need to check each one
    if (type.uniqueItems)
    {
      for (unsigned int checkingIndex = 0; checkingIndex < outputValue.size(); checkingIndex++)
      {
        for (unsigned int checkedIndex = checkingIndex + 1; checkedIndex < outputValue.size(); checkedIndex++)
        {
          // If two elements are the same they are not unique
          if (outputValue[checkingIndex] == outputValue[checkedIndex])
          {
            CLog::Log(LOGWARNING, "JSONRPC: Not unique array element at index %u and %u in type %s", checkingIndex, checkedIndex, type.name.c_str());
            errorMessage.Format("Array element at index %u is not unique (same as array element at index %u)", checkingIndex, checkedIndex);
            errorData["message"] = errorMessage.c_str();
            return InvalidParams;
          }
        }
      }
    }

    return OK;
  }

  // If it is an object we need to check every element
  // against the defined "properties"
  if (HasType(type.type, ObjectValue) && value.isObject())
  {
    unsigned int handled = 0;
    JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator propertiesEnd = type.properties.end();
    JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator propertiesIterator;
    for (propertiesIterator = type.properties.begin(); propertiesIterator != propertiesEnd; propertiesIterator++)
    {
      if (value.isMember(propertiesIterator->first))
      {
        JSON_STATUS status = checkType(value[propertiesIterator->first], propertiesIterator->second, outputValue[propertiesIterator->first], errorData["property"]);
        if (status != OK)
        {
          CLog::Log(LOGWARNING, "JSONRPC: Invalid property \"%s\" in type %s", propertiesIterator->first.c_str(), type.name.c_str());
          return status;
        }
        handled++;
      }
      else if (propertiesIterator->second.optional)
        outputValue[propertiesIterator->first] = propertiesIterator->second.defaultValue;
      else
      {
        CLog::Log(LOGWARNING, "JSONRPC: Missing property \"%s\" in type %s", propertiesIterator->first.c_str(), type.name.c_str());
        errorData["property"]["name"] = propertiesIterator->first.c_str();
        errorData["property"]["type"] = SchemaValueTypeToString(propertiesIterator->second.type);
        errorData["message"] = "Missing property";
        return InvalidParams;
      }
    }

    // Additional properties are not allowed
    if (handled < value.size())
    {
      errorData["message"] = "Unexpected additional properties received";
      return InvalidParams;
    }

    return OK;
  }

  // It's neither an array nor an object

  // If it can only take certain values ("enum")
  // we need to check against those
  if (type.enums.size() > 0)
  {
    bool valid = false;
    for (std::vector<CVariant>::const_iterator enumItr = type.enums.begin(); enumItr != type.enums.end(); enumItr++)
    {
      if (*enumItr == value)
      {
        valid = true;
        break;
      }
    }

    if (!valid)
    {
      CLog::Log(LOGWARNING, "JSONRPC: Value does not match any of the enum values in type %s", type.name.c_str());
      errorData["message"] = "Received value does not match any of the defined enum values";
      return InvalidParams;
    }
  }

  // If we have a number or an integer type, we need
  // to check the minimum and maximum values
  if ((HasType(type.type, NumberValue) || HasType(type.type, IntegerValue)) && value.isDouble())
  {
    double numberValue = value.asDouble();
    // Check minimum
    if ((type.exclusiveMinimum && numberValue <= type.minimum) || (!type.exclusiveMinimum && numberValue < type.minimum) ||
    // Check maximum
        (type.exclusiveMaximum && numberValue >= type.maximum) || (!type.exclusiveMaximum && numberValue > type.maximum))        
    {
      CLog::Log(LOGWARNING, "JSONRPC: Value does not lay between minimum and maximum in type %s", type.name.c_str());
      errorMessage.Format("Value between %f (%s) and %f (%s) expected but %f received", 
        type.minimum, type.exclusiveMinimum ? "exclusive" : "inclusive", type.maximum, type.exclusiveMaximum ? "exclusive" : "inclusive", numberValue);
      errorData["message"] = errorMessage.c_str();
      return InvalidParams;
    }
    // Check divisibleBy
    else if ((HasType(type.type, IntegerValue) && type.divisibleBy > 0 && ((int)numberValue % type.divisibleBy) != 0))
    {
      CLog::Log(LOGWARNING, "JSONRPC: Value does not meet divisibleBy requirements in type %s", type.name.c_str());
      errorMessage.Format("Value should be divisible by %d but %d received", type.divisibleBy, (int)numberValue);
      errorData["message"] = errorMessage.c_str();
      return InvalidParams;
    }
  }

  // Otherwise it can have any value
  outputValue = value;
  return OK;
}

void CJSONServiceDescription::parseHeader(const CVariant &descriptionObject)
{
  m_header.ID = GetString(descriptionObject["id"], "");
  m_header.version = descriptionObject["version"].asInteger(0);
  m_header.description = GetString(descriptionObject["description"], "");
}

bool CJSONServiceDescription::parseMethod(const CVariant &value, JsonRpcMethod &method)
{
  // Parse XBMC specific information about the method
  method.transportneed = StringToTransportLayer(value.isMember("transport") ? value["transport"].asString() : "");
  method.permission = StringToPermission(value.isMember("permission") ? value["permission"].asString() : "");
  method.description = GetString(value["description"], "");

  // Check whether there are parameters defined
  if (value.isMember("params") && value["params"].isArray())
  {
    // Loop through all defined parameters
    for (unsigned int paramIndex = 0; paramIndex < value["params"].size(); paramIndex++)
    {
      CVariant parameter = value["params"][paramIndex];
      // If the parameter definition does not contain a valid "name" or
      // "type" element we will ignore it
      if (!parameter.isMember("name") || !parameter["name"].isString() ||
          (!parameter.isMember("type") && !parameter.isMember("$ref")) ||
          (parameter.isMember("type") && !parameter["type"].isString() &&
          !parameter["type"].isArray()) || (parameter.isMember("$ref") &&
          !parameter["$ref"].isString()))
      {
        CLog::Log(LOGWARNING, "JSONRPC: Method %s has a badly defined parameter", method.name.c_str());
        return false;
      }

      // Parse the parameter and add it to the list
      // of defined parameters
      JSONSchemaTypeDefinition param;
      if (!parseParameter(parameter, param))
        return false;
      method.parameters.push_back(param);
    }
  }
    
  // Parse the return value of the method
  parseReturn(value, method.returns);

  return true;
}

bool CJSONServiceDescription::parseParameter(CVariant &value, JSONSchemaTypeDefinition &parameter)
{
  parameter.name = GetString(value["name"], "");

  // Parse the type and default value of the parameter
  return parseTypeDefinition(value, parameter, true);
}

bool CJSONServiceDescription::parseTypeDefinition(const CVariant &value, JSONSchemaTypeDefinition &type, bool isParameter)
{
  bool isReferenceType = false;
  bool hasReference = false;

  // Check if the type of the parameter defines a json reference
  // to a type defined somewhere else
  if (value.isMember("$ref") && value["$ref"].isString())
  {
    // Get the name of the referenced type
    std::string refType = value["$ref"].asString();
    // Check if the referenced type exists
    std::map<std::string, JSONSchemaTypeDefinition>::const_iterator iter = m_types.find(refType);
    if (refType.length() <= 0 || iter == m_types.end())
    {
      CLog::Log(LOGDEBUG, "JSONRPC: JSON schema type %s references an unknown type %s", type.name.c_str(), refType.c_str());
      return false;
    }
    
    std::string typeName = type.name;
    type = iter->second;
    if (!typeName.empty())
      type.name = typeName;
    hasReference = true;
  }
  else if (value.isMember("id") && value["id"].isString())
  {
    type.ID = GetString(value["id"], "");
    isReferenceType = true;
  }

  // Check if the "required" field has been defined
  type.optional = value.isMember("required") && value["required"].isBoolean() ? !value["required"].asBoolean() : true;

  // Get the "description"
  if (!hasReference || (value.isMember("description") && value["description"].isString()))
    type.description = GetString(value["description"], "");

  if (hasReference)
  {
    // If there is a specific default value, read it
    if (value.isMember("default") && IsType(value["default"], type.type))
    {
      bool ok = false;
      if (type.enums.size() >= 0)
        ok = true;
      // If the type has an enum definition we must make
      // sure that the default value is a valid enum value
      else
      {
        for (std::vector<CVariant>::const_iterator itr = type.enums.begin(); itr != type.enums.end(); itr++)
        {
          if (value["default"] == *itr)
          {
            ok = true;
            break;
          }
        }
      }

      if (ok)
        type.defaultValue = value["default"];
    }

    return true;
  }

  // Get the defined type of the parameter
  if (value["type"].isArray())
  {
    int parsedType = 0;
    // If the defined type is an array, we have
    // to handle a union type
    for (unsigned int typeIndex = 0; typeIndex < value["type"].size(); typeIndex++)
    {
      // If the type is a string try to parse it
      if (value["type"][typeIndex].isString())
        parsedType |= StringToSchemaValueType(value["type"][typeIndex].asString());
      else
        CLog::Log(LOGWARNING, "JSONRPC: Invalid type in union type definition of type %s", type.name.c_str());
    }

    type.type = (JSONSchemaType)parsedType;

    // If the type has not been set yet
    // set it to "any"
    if (type.type == 0)
      type.type = AnyValue;
  }
  else
    type.type = value["type"].isString() ? StringToSchemaValueType(value["type"].asString()) : AnyValue;

  if (type.type == ObjectValue)
  {
    // If the type definition is of type "object"
    // and has a "properties" definition we need
    // to handle these as well
    if (value.isMember("properties") && value["properties"].isObject())
    {
      // Get all child elements of the "properties"
      // object and loop through them
      for (CVariant::const_iterator_map itr = value["properties"].begin_map(); itr != value["properties"].end_map(); itr++)
      {
        // Create a new type definition, store the name
        // of the current property into it, parse it
        // recursively and add its default value
        // to the current type's default value
        JSONSchemaTypeDefinition propertyType;
        propertyType.name = itr->first;
        if (!parseTypeDefinition(itr->second, propertyType, false))
          return false;
        type.defaultValue[itr->first] = propertyType.defaultValue;
        type.properties.add(propertyType);
      }
    }
  }
  else 
  {
    // If the defined parameter is an array
    // we need to check for detailed definitions
    // of the array items
    if (type.type == ArrayValue)
    {
      // Check for "uniqueItems" field
      if (value.isMember("uniqueItems") && value["uniqueItems"].isBoolean())
        type.uniqueItems = value["uniqueItems"].asBoolean();
      else
        type.uniqueItems = false;

      // Check for "additionalItems" field
      if (value.isMember("additionalItems"))
      {
        // If it is an object, there is only one schema for it
        if (value["additionalItems"].isObject())
        {
          JSONSchemaTypeDefinition additionalItem;

          if (parseTypeDefinition(value["additionalItems"], additionalItem, false))
            type.additionalItems.push_back(additionalItem);
        }
        // If it is an array there may be multiple schema definitions
        else if (value["additionalItems"].isArray())
        {
          for (unsigned int itemIndex = 0; itemIndex < value["additionalItems"].size(); itemIndex++)
          {
            JSONSchemaTypeDefinition additionalItem;

            if (parseTypeDefinition(value["additionalItems"][itemIndex], additionalItem, false))
              type.additionalItems.push_back(additionalItem);
          }
        }
        // If it is not a (array of) schema and not a bool (default value is false)
        // it has an invalid value
        else if (!value["additionalItems"].isBoolean())
          CLog::Log(LOGWARNING, "Invalid \"additionalItems\" value for type %s", type.name.c_str());
      }

      // If the "items" field is a single object
      // we can parse that directly
      if (value.isMember("items"))
      {
        if (value["items"].isObject())
        {
          JSONSchemaTypeDefinition item;

          if (!parseTypeDefinition(value["items"], item, false))
            return false;
          type.items.push_back(item);
        }
        // Otherwise if it is an array we need to
        // parse all elements and store them
        else if (value["items"].isArray())
        {
          for (CVariant::const_iterator_array itemItr = value["items"].begin_array(); itemItr != value["items"].end_array(); itemItr++)
          {
            JSONSchemaTypeDefinition item;

            if (!parseTypeDefinition(*itemItr, item, false))
              return false;
            type.items.push_back(item);
          }
        }
      }

      type.minItems = value["minItems"].asInteger(0);
      type.maxItems = value["maxItems"].asInteger(0);
    }
    // The type is whether an object nor an array
    else 
    {
      if ((type.type & NumberValue) == NumberValue || (type.type  & IntegerValue) == IntegerValue)
      {
        if ((type.type & NumberValue) == NumberValue)
        {
          type.minimum = value["minimum"].asDouble(-numeric_limits<double>::max());
          type.maximum = value["maximum"].asDouble(numeric_limits<double>::max());
        }
        else if ((type.type  & IntegerValue) == IntegerValue)
        {
          type.minimum = value["minimum"].asInteger(numeric_limits<int>::min());
          type.maximum = value["maximum"].asInteger(numeric_limits<int>::max());
        }

        type.exclusiveMinimum = value["exclusiveMinimum"].asBoolean(false);
        type.exclusiveMaximum = value["exclusiveMaximum"].asBoolean(false);
        type.divisibleBy = value["divisibleBy"].asUnsignedInteger(0);
      }

      // If the type definition is neither an
      // "object" nor an "array" we can check
      // for an "enum" definition
      if (value.isMember("enum") && value["enum"].isArray())
      {
        // Loop through all elements in the "enum" array
        for (CVariant::const_iterator_array enumItr = value["enum"].begin_array(); enumItr != value["enum"].end_array(); enumItr++)
        {
          // Check for duplicates and eliminate them
          bool approved = true;
          for (unsigned int approvedIndex = 0; approvedIndex < type.enums.size(); approvedIndex++)
          {
            if (*enumItr == type.enums.at(approvedIndex))
            {
              approved = false;
              break;
            }
          }

          // Only add the current item to the enum value 
          // list if it is not duplicate
          if (approved)
            type.enums.push_back(*enumItr);
        }
      }
    }

    // If there is a definition for a default value and its type
    // matches the type of the parameter we can parse it
    bool ok = false;
    if (value.isMember("default") && IsType(value["default"], type.type))
    {
      if (type.enums.size() >= 0)
        ok = true;
      // If the type has an enum definition we must make
      // sure that the default value is a valid enum value
      else
      {
        for (std::vector<CVariant>::const_iterator itr = type.enums.begin(); itr != type.enums.end(); itr++)
        {
          if (value["default"] == *itr)
          {
            ok = true;
            break;
          }
        }
      }
    }

    if (ok)
      type.defaultValue = value["default"];
    else
    {
      // If the type of the default value definition does not
      // match the type of the parameter we have to log this
      if (value.isMember("default") && !IsType(value["default"], type.type))
        CLog::Log(LOGWARNING, "JSONRPC: Parameter %s has an invalid default value", type.name.c_str());
      
      // If the type contains an "enum" we need to get the
      // default value from the first enum value
      if (type.enums.size() > 0)
        type.defaultValue = type.enums.at(0);
      // otherwise set a default value instead
      else
        SetDefaultValue(type.defaultValue, type.type);
    }
  }

  if (isReferenceType)
    addReferenceTypeDefinition(type);

  return true;
}

void CJSONServiceDescription::parseReturn(const CVariant &value, CVariant &returns)
{
  // Only parse the "returns" definition if there is one
  if (value.isMember("returns"))
    returns = value["returns"];
}

void CJSONServiceDescription::addReferenceTypeDefinition(JSONSchemaTypeDefinition &typeDefinition)
{
  // If the given json value is no object or does not contain an "id" field
  // of type string it is no valid type definition
  if (typeDefinition.ID.empty())
    return;

  // If the id has already been defined we ignore the type definition
  if (m_types.find(typeDefinition.ID) != m_types.end())
    return;

  // Add the type to the list of type definitions
  m_types[typeDefinition.ID] = typeDefinition;
  if (m_unresolvedMethods.size() > 0)
    m_newReferenceType = true;
}

CJSONServiceDescription::CJsonRpcMethodMap::CJsonRpcMethodMap()
{
  m_actionmap = std::map<std::string, JsonRpcMethod>();
}

void CJSONServiceDescription::CJsonRpcMethodMap::add(JsonRpcMethod &method)
{
  CStdString name = method.name;
  name = name.ToLower();
  m_actionmap[name] = method;
}

CJSONServiceDescription::CJsonRpcMethodMap::JsonRpcMethodIterator CJSONServiceDescription::CJsonRpcMethodMap::begin() const
{
  return m_actionmap.begin();
}

CJSONServiceDescription::CJsonRpcMethodMap::JsonRpcMethodIterator CJSONServiceDescription::CJsonRpcMethodMap::find(const std::string& key) const
{
  return m_actionmap.find(key);
}

CJSONServiceDescription::CJsonRpcMethodMap::JsonRpcMethodIterator CJSONServiceDescription::CJsonRpcMethodMap::end() const
{
  return m_actionmap.end();
}
