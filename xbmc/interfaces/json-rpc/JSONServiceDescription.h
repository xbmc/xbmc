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

#include <string>
#include <vector>
#include "jsoncpp/include/json/json.h"
#include "JSONUtils.h"

namespace JSONRPC
{
  /*! 
   \ingroup jsonrpc
   \brief Structure for the header of 
   of a service mapping description.
   
   Represents the header of a service mapping
   description containing its version, id,
   description, transport, envelope, contentType,
   target and additionalParameters fields.
   */
  typedef struct
  {
    /*!
     \brief Identification of the published
     service containing json rpc methods.
     Renamed from "id" because of possible
     issues with Objective-C.
     */
    std::string ID;
    /*!
     \brief Description of the published
     service containing json rpc methods.
     */
    std::string description;

    /*!
     \brief Version of the published
     service containing json rpc methods.
     */
    int version;
  } JsonRpcDescriptionHeader;

  /*! 
   \ingroup jsonrpc
   \brief Structure for a parameter of a
   json rpc method.
   
   Represents a parameter of a defined
   json rpc method and is used to verify
   and extract the value of the parameter
   in a method call.
   */
  typedef struct JSONSchemaTypeDefinition
  {
    /*!
     \brief Name of the parameter (for 
     by-name calls)
     */
    std::string name;

    /*!
     \brief Id of the type (for
     referenced types)
     Renamed from "id" because of possible
     issues with Objective-C.
     */
    std::string ID;

    /*!
     \brief Description of the parameter
     */
    std::string description;

    /*!
     \brief JSON schema type of the parameter's value
     */
    JSONSchemaType type;

    /*!
     \brief Whether or not the parameter is
     optional
     */
    bool optional;

    /*!
     \brief Default value of the parameter
     (only needed when it is optional)
     */
    Json::Value defaultValue;

    /*!
     \brief Minimum value for Integer
     or Number types
     */
    double minimum;

    /*!
     \brief Maximum value for Integer or Number types
     */
    double maximum;

    /*!
     \brief Whether to exclude the defined Minimum
     value from the valid range or not
     */
    bool exclusiveMinimum;

    /*!
     \brief  Whether to exclude the defined Maximum
     value from the valid range or not
     */
    bool exclusiveMaximum;

    /*!
     \brief Integer by which the value (of type
     Integer) must be divisible without rest
     */
    unsigned int divisibleBy;

    /*!
     \brief (Optional) List of allowed values
     for the type
     */
    std::vector<Json::Value> enums;

    /*!
     \brief List of possible values in an array
     */
    std::vector<JSONSchemaTypeDefinition> items;

    /*!
     \brief Minimum amount of items in the array
     */
    unsigned minItems;

    /*!
     \brief Maximum amount of items in the array
     */
    unsigned maxItems;

    /*!
     \brief Whether every value in the array
     must be unique or not
     */
    bool uniqueItems;

    /*!
     \brief List of json schema definitions for
     additional items in an array with tuple
     typing (defined schemas in "items")
     */
    std::vector<JSONSchemaTypeDefinition> additionalItems;

    /*!
     \brief Maps a properties name to its
     json schema type definition
     */
    class CJsonSchemaPropertiesMap
    {
    public:
      CJsonSchemaPropertiesMap();

      void add(JSONSchemaTypeDefinition &property);

      typedef std::map<std::string, JSONSchemaTypeDefinition>::const_iterator JSONSchemaPropertiesIterator;
      JSONSchemaPropertiesIterator begin() const;
      JSONSchemaPropertiesIterator find(const std::string& key) const;
      JSONSchemaPropertiesIterator end() const;
      unsigned int size() const;
    private:
      std::map<std::string, JSONSchemaTypeDefinition> m_propertiesmap;
    };

    /*!
     \brief List of properties of the parameter (only needed when the
     parameter is an object)
     */
    CJsonSchemaPropertiesMap properties;
  } JSONSchemaTypeDefinition;

  /*! 
   \ingroup jsonrpc
   \brief Structure for a published json
   rpc method.
   
   Represents a published json rpc method
   and is used to verify an incoming json
   rpc request against a defined method.
   */
  typedef struct
  {
    /*!
     \brief Name of the represented method
     */
    std::string name;
    /*!
     \brief Pointer tot he implementation
     of the represented method
     */
    MethodCall method;
    /*!
     \brief Definition of the type of
     request/response
     */
    TransportLayerCapability transportneed;
    /*!
     \brief Definition of the permissions needed
     to execute the method
     */
    OperationPermission permission;
    /*!
     \brief Description of the method
     */
    std::string description;
    /*!
     \brief List of accepted parameters
     */
    std::vector<JSONSchemaTypeDefinition> parameters;
    /*!
     \brief Definition of the return value
     */
    Json::Value returns;
  } JsonRpcMethod;

  /*! 
   \ingroup jsonrpc
   \brief Structure mapping a json rpc method
   definition to an actual method implementation.
   */
  typedef struct
  {
    /*!
     \brief Name of the json rpc method.
     */
    std::string name;
    /*!
     \brief Pointer to the actual
     implementation of the json rpc
     method.
     */
    MethodCall method;
  } JsonRpcMethodMap;

  /*!
   \ingroup jsonrpc
   \brief Helper class for json schema service descriptor based
   service descriptions for the json rpc API

   Provides static functions to parse a complete json schema
   service descriptor of a published service containing json rpc 
   methods, print the json schema service descriptor representation 
   into a string (mainly for output purposes) and evaluate and verify 
   parameters provided in a call to one of the publish json rpc methods 
   against a parameter definition parsed from a json schema service
   descriptor.
   */
  class CJSONServiceDescription : public CJSONUtils
  {
  public:
    /*!
     \brief Parses the given json schema description and maps
     the found methods against the given method map
     \param serviceDescription json schema description to parse
     \param methodMap Map of json rpc method names to actual C/C++ implementations
     \param size Size of the method map
     \return True if the json schema description has been parsed sucessfully otherwise false
     */
    static bool Parse(JsonRpcMethodMap methodMap[], unsigned int size);

    /*!
     \brief Gets the version of the json
     schema description
     \return Version of the json schema description
     */
    static int GetVersion();

    /*!
     \brief Prints the json schema description into the given result object
     \param result Object into which the json schema description is printed
     \param transport Transport layer capabilities
     \param client Client requesting a print
     \param printDescriptions Whether to print descriptions or not
     \param printMetadata Whether to print XBMC specific data or not
     \param filterByTransport Whether to filter by transport or not
     */
    static void Print(Json::Value &result, ITransportLayer *transport, IClient *client, bool printDescriptions, bool printMetadata, bool filterByTransport);

    /*!
     \brief Checks the given parameters from the request against the
     json schema description for the given method
     \param method Called method
     \param requestParameters Parameters from the request
     \param client Client who sent the request
     \param notification Whether the request was sent as a notification or not
     \param methodCall Object which will contain the actual C/C++ method to be called
     \param outputParameters Cleaned up parameter list
     \return OK if the validation of the request succeeded otherwise an appropriate error code

     Checks if the given method is a valid json rpc method, if the client has the permission
     to call this method, if the method can be called as a notification or not, assigns the
     actual C/C++ implementation of the method to the "methodCall" parameter and checks the
     given parameters from the request against the json schema description for the given method.
     */
    static JSON_STATUS CheckCall(const char* const method, const Json::Value &requestParameters, IClient *client, bool notification, MethodCall &methodCall, Json::Value &outputParameters);

  private:
    static void printType(const JSONSchemaTypeDefinition &type, bool isParameter, bool isGlobal, bool printDefault, bool printDescriptions, Json::Value &output);
    static JSON_STATUS checkParameter(const Json::Value &requestParameters, const JSONSchemaTypeDefinition &type, unsigned int position, Json::Value &outputParameters, unsigned int &handled, Json::Value &errorData);
    static JSON_STATUS checkType(const Json::Value &value, const JSONSchemaTypeDefinition &type, Json::Value &outputValue, Json::Value &errorData);
    static void parseHeader(const Json::Value &descriptionObject);
    static bool parseMethod(const Json::Value &value, JsonRpcMethod &method);
    static bool parseParameter(Json::Value &value, JSONSchemaTypeDefinition &parameter);
    static bool parseTypeDefinition(const Json::Value &value, JSONSchemaTypeDefinition &type, bool isParameter);
    static void parseReturn(const Json::Value &value, Json::Value &returns);
    static void addReferenceTypeDefinition(JSONSchemaTypeDefinition &typeDefinition);

    class CJsonRpcMethodMap
    {
    public:
      CJsonRpcMethodMap();

      void add(JsonRpcMethod &method);

      typedef std::map<std::string, JsonRpcMethod>::const_iterator JsonRpcMethodIterator;
      JsonRpcMethodIterator begin() const;
      JsonRpcMethodIterator find(const std::string& key) const;
      JsonRpcMethodIterator end() const;
    private:
      std::map<std::string, JsonRpcMethod> m_actionmap;
    };

    static Json::Value m_notifications;
    static JsonRpcDescriptionHeader m_header;
    static CJsonRpcMethodMap m_actionMap;
    static std::map<std::string, JSONSchemaTypeDefinition> m_types;
    static std::vector<JsonRpcMethodMap> m_unresolvedMethods;
    static bool m_newReferenceType;
  };
}
