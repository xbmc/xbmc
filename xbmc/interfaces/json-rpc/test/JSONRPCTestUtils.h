/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ServiceDescription.h"
#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "interfaces/json-rpc/JSONRPCUtils.h"
#include "interfaces/json-rpc/JSONServiceDescription.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace JSONRPC
{

class CAllCapabilityTransport : public ITransportLayer
{
public:
  bool PrepareDownload(const char* path, CVariant& details, std::string& protocol) override
  {
    return false;
  }
  bool Download(const char* path, CVariant& result) override { return false; }
  int GetCapabilities() override { return TRANSPORT_LAYER_CAPABILITY_ALL; }
};

class CAllPermissionClient : public IClient
{
public:
  int GetPermissionFlags() override { return OPERATION_PERMISSION_ALL; }
  int GetAnnouncementFlags() override { return 0; }
  bool SetAnnouncementFlags(int flags) override { return true; }
};

inline JSONRPC_STATUS StubMethod(const std::string& method,
                                 ITransportLayer* transport,
                                 IClient* client,
                                 const CVariant& parameterObject,
                                 CVariant& result)
{
  return OK;
}

inline CVariant ParseJson(const std::string& json)
{
  CVariant parsed;
  EXPECT_TRUE(CJSONVariantParser::Parse(json, parsed)) << "invalid test JSON: " << json;
  return parsed;
}

inline std::string ToJson(const CVariant& variant)
{
  std::string json;
  CJSONVariantWriter::Write(variant, json, true);
  return json;
}

//! \brief Every definition in one of the JSONRPC_SERVICE_* tables, by name
inline std::map<std::string, CVariant> ShippedDefinitions(const char* const entries[], size_t count)
{
  std::map<std::string, CVariant> definitions;
  for (size_t index = 0; index < count; ++index)
  {
    // Each entry is one definition without its enclosing braces
    CVariant parsed;
    if (!CJSONVariantParser::Parse("{" + std::string(entries[index]) + "}", parsed))
      continue;

    for (auto member = parsed.begin_map(); member != parsed.end_map(); ++member)
      definitions.emplace(member->first, member->second);
  }

  return definitions;
}

/*!
 \brief One definition out of a JSONRPC_SERVICE_* table, by name.

 Read rather than restated, so that a declaration which never reaches the schema fails the test.
 */
inline CVariant ShippedDefinition(const char* const entries[],
                                  size_t count,
                                  const std::string& name)
{
  for (size_t index = 0; index < count; ++index)
  {
    CVariant parsed;
    if (!CJSONVariantParser::Parse("{" + std::string(entries[index]) + "}", parsed))
      continue;

    if (parsed.isMember(name))
      return parsed[name];
  }

  ADD_FAILURE() << name << " is not declared in the service description";
  return {};
}

inline CVariant ShippedMethod(const std::string& name)
{
  return ShippedDefinition(JSONRPC_SERVICE_METHODS, std::size(JSONRPC_SERVICE_METHODS), name);
}

inline CVariant ShippedNotification(const std::string& name)
{
  return ShippedDefinition(JSONRPC_SERVICE_NOTIFICATIONS, std::size(JSONRPC_SERVICE_NOTIFICATIONS),
                           name);
}

inline CVariant ShippedType(const std::string& name)
{
  return ShippedDefinition(JSONRPC_SERVICE_TYPES, std::size(JSONRPC_SERVICE_TYPES), name);
}

inline std::map<std::string, CVariant> ShippedMethods()
{
  return ShippedDefinitions(JSONRPC_SERVICE_METHODS, std::size(JSONRPC_SERVICE_METHODS));
}

inline std::map<std::string, CVariant> ShippedTypes()
{
  return ShippedDefinitions(JSONRPC_SERVICE_TYPES, std::size(JSONRPC_SERVICE_TYPES));
}

//! \brief A type's entry as JSON text, ready for CJSONServiceDescription::AddType
inline std::string ShippedDefinition(const std::string& type)
{
  CVariant wrapper{CVariant::VariantTypeObject};
  wrapper[type] = ShippedType(type);
  return ToJson(wrapper);
}

//! \brief The values of a schema's "enum"
inline std::set<std::string> EnumValues(const CVariant& schema)
{
  std::set<std::string> values;
  const CVariant& list{schema["enum"]};
  for (auto value = list.begin_array(); value != list.end_array(); ++value)
    values.insert(value->asString());
  return values;
}

//! \brief The names an object schema's "required" lists
inline std::set<std::string> RequiredMembers(const CVariant& object)
{
  std::set<std::string> required;
  const CVariant& values{object["required"]};
  for (auto value = values.begin_array(); value != values.end_array(); ++value)
    required.insert(value->asString());
  return required;
}

//! \brief The member names of an object
inline std::set<std::string> Keys(const CVariant& object)
{
  std::set<std::string> keys;
  for (auto it = object.begin_map(); it != object.end_map(); ++it)
    keys.insert(it->first);
  return keys;
}

//! \brief A method's parameter descriptors, by name
inline std::map<std::string, CVariant> Params(const CVariant& method)
{
  std::map<std::string, CVariant> params;
  const CVariant& values{method["params"]};
  for (auto value = values.begin_array(); value != values.end_array(); ++value)
    params.emplace((*value)["name"].asString(), *value);
  return params;
}

//! \brief One parameter descriptor of a method, or nullptr when it declares none by that name
inline const CVariant* Param(const CVariant& method, const std::string& name)
{
  const CVariant& params{method["params"]};
  for (auto param = params.begin_array(); param != params.end_array(); ++param)
  {
    if (param->isMember("name") && (*param)["name"].asString() == name)
      return &(*param);
  }
  return nullptr;
}

//! \brief The enum types CJSONRPC::Initialize registers from C++ tables at runtime
inline const std::vector<std::string>& RuntimeEnumNames()
{
  static const std::vector<std::string> names{
      "Addon.Types",
      "Input.Action",
      "GUI.Window",
      "List.Filter.Operators",
      "List.Filter.Fields.Movies",
      "List.Filter.Fields.TVShows",
      "List.Filter.Fields.Episodes",
      "List.Filter.Fields.MusicVideos",
      "List.Filter.Fields.Artists",
      "List.Filter.Fields.Albums",
      "List.Filter.Fields.Songs",
      "List.Filter.Fields.Textures",
  };
  return names;
}

//! \brief Loads the whole shipped service description, with a placeholder for each runtime enum
inline void AddShippedServiceDescription()
{
  for (const std::string& name : RuntimeEnumNames())
    ASSERT_TRUE(CJSONServiceDescription::AddEnum(name, std::vector<std::string>{"placeholder"}));

  for (const char* const entry : JSONRPC_SERVICE_TYPES)
    CJSONServiceDescription::AddType(entry);

  for (const char* const entry : JSONRPC_SERVICE_METHODS)
    CJSONServiceDescription::AddBuiltinMethod(entry);

  for (const char* const entry : JSONRPC_SERVICE_NOTIFICATIONS)
    CJSONServiceDescription::AddNotification(entry);

  CJSONServiceDescription::ResolveReferences();
}

/*!
 \brief Fixture isolating the global schema registry of CJSONServiceDescription.
 */
class JSONServiceDescriptionTestBase : public ::testing::Test
{
public:
  void SetUp() override { CJSONServiceDescription::Cleanup(); }
  void TearDown() override { CJSONServiceDescription::Cleanup(); }

  JSONRPC_STATUS Call(const char* method, const std::string& paramsJson, CVariant& output)
  {
    // the transport layer lowercases the method before dispatch
    std::string key = method;
    StringUtils::ToLower(key);
    MethodCall call = nullptr;
    output = CVariant();
    return CJSONServiceDescription::CheckCall(key.c_str(), ParseJson(paramsJson), &m_transport,
                                              &m_client, false, call, output);
  }

  CAllCapabilityTransport m_transport;
  CAllPermissionClient m_client;
};

} // namespace JSONRPC
