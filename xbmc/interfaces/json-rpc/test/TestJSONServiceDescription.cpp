/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "interfaces/json-rpc/JSONServiceDescription.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
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

JSONRPC_STATUS StubMethod(const CVariant& parameterObject, CVariant& result)
{
  return OK;
}

CVariant ParseJson(const std::string& json)
{
  CVariant parsed;
  EXPECT_TRUE(CJSONVariantParser::Parse(json, parsed)) << "invalid test JSON: " << json;
  return parsed;
}

std::string ToJson(const CVariant& variant)
{
  std::string json;
  CJSONVariantWriter::Write(variant, json, true);
  return json;
}

void ExpectVariantEq(const CVariant& expected, const CVariant& actual)
{
  EXPECT_TRUE(expected == actual) << "expected: " << ToJson(expected)
                                  << "\n  actual: " << ToJson(actual);
}

//! \brief One definition written both ways: the draft-03 form and its JSON Schema 2020-12 twin
struct Twin
{
  std::string draft03;
  std::string draft2020;
};

// clang-format off
const std::vector<Twin> TWIN_TYPES = {
  { R"({"Twin.Id": { "type": "integer", "default": -1, "minimum": 1 }})",
    R"({"Twin.Id": { "type": "integer", "default": -1, "minimum": 1 }})" },
  { R"({"Twin.Base": { "type": "object", "description": "a base", "properties": {
        "a": { "type": "string", "required": true },
        "shared": { "type": "integer", "default": 1 } } }})",
    R"({"Twin.Base": { "type": "object", "description": "a base", "properties": {
        "a": { "type": "string" },
        "shared": { "type": "integer", "default": 1 } },
        "required": ["a"] }})" },
  { R"({"Twin.Other": { "type": "object", "properties": { "c": { "type": "number" } } }})",
    R"({"Twin.Other": { "type": "object", "properties": { "c": { "type": "number" } } }})" },
  { R"({"Twin.Derived": { "extends": "Twin.Base", "properties": {
        "b": { "type": "boolean", "required": true },
        "ref": { "$ref": "Twin.Id", "required": true },
        "shared": { "type": "integer", "default": 2 } } }})",
    R"({"Twin.Derived": { "allOf": [ { "$ref": "#/$defs/Twin.Base" } ], "properties": {
        "b": { "type": "boolean" },
        "ref": { "$ref": "#/$defs/Twin.Id" },
        "shared": { "type": "integer", "default": 2 } },
        "required": ["b", "ref"] }})" },
  { R"({"Twin.Both": { "extends": [ "Twin.Base", "Twin.Other" ] }})",
    R"({"Twin.Both": { "allOf": [ { "$ref": "#/$defs/Twin.Base" }, { "$ref": "#/$defs/Twin.Other" } ] }})" },
  { R"({"Twin.Union": { "type": [
        { "type": "object", "properties": { "movieid": { "$ref": "Twin.Id", "required": true } },
          "additionalProperties": false },
        { "type": "object", "properties": { "songid": { "$ref": "Twin.Id", "required": true } },
          "additionalProperties": false } ] }})",
    R"({"Twin.Union": { "anyOf": [
        { "type": "object", "properties": { "movieid": { "$ref": "#/$defs/Twin.Id" } },
          "required": ["movieid"], "additionalProperties": false },
        { "type": "object", "properties": { "songid": { "$ref": "#/$defs/Twin.Id" } },
          "required": ["songid"], "additionalProperties": false } ] }})" },
  { R"({"Twin.Optional": { "type": [ "null", { "$ref": "Twin.Base" } ], "default": null }})",
    R"({"Twin.Optional": { "anyOf": [ { "type": "null" }, { "$ref": "#/$defs/Twin.Base" } ], "default": null }})" },
  { R"({"Twin.Any": { "type": "object", "properties": { "data": { "type": "any", "default": null } } }})",
    R"({"Twin.Any": { "type": "object", "properties": { "data": { "default": null } } }})" },
  { R"({"Twin.List": { "type": "array", "items": { "$ref": "Twin.Id" }, "uniqueItems": true, "minItems": 1 }})",
    R"({"Twin.List": { "type": "array", "items": { "$ref": "#/$defs/Twin.Id" }, "uniqueItems": true, "minItems": 1 }})" },
  { R"({"Twin.Map": { "type": "object", "additionalProperties": { "$ref": "Twin.Id" } }})",
    R"({"Twin.Map": { "type": "object", "additionalProperties": { "$ref": "#/$defs/Twin.Id" } }})" },
};

const Twin TWIN_METHOD = {
  R"({"Twin.Method": { "type": "method", "description": "test", "transport": "Response",
      "permission": "ReadData", "returns": { "$ref": "Twin.List" }, "params": [
      { "name": "target", "$ref": "Twin.Union", "required": true },
      { "name": "item", "$ref": "Twin.Derived" },
      { "name": "both", "$ref": "Twin.Both" },
      { "name": "opt", "$ref": "Twin.Optional" },
      { "name": "level", "$ref": "Twin.Id", "default": 7 },
      { "name": "filter", "type": "object", "description": "a filter", "properties": {
          "path": { "type": "string", "required": true },
          "mode": { "type": "string", "enum": ["fast", "slow"], "default": "fast" } },
        "additionalProperties": false },
      { "name": "flag", "type": [ "null", "boolean" ], "default": null },
      { "name": "data", "type": "any", "default": null },
      { "name": "map", "$ref": "Twin.Map" } ] }})",
  R"({"Twin.Method": { "type": "method", "description": "test", "transport": "Response",
      "permission": "ReadData", "returns": { "$ref": "#/$defs/Twin.List" }, "params": [
      { "name": "target", "required": true, "schema": { "$ref": "#/$defs/Twin.Union" } },
      { "name": "item", "schema": { "$ref": "#/$defs/Twin.Derived" } },
      { "name": "both", "schema": { "$ref": "#/$defs/Twin.Both" } },
      { "name": "opt", "schema": { "$ref": "#/$defs/Twin.Optional" } },
      { "name": "level", "schema": { "$ref": "#/$defs/Twin.Id", "default": 7 } },
      { "name": "filter", "description": "a filter", "schema": { "type": "object", "properties": {
          "path": { "type": "string" },
          "mode": { "type": "string", "enum": ["fast", "slow"], "default": "fast" } },
        "required": ["path"], "additionalProperties": false } },
      { "name": "flag", "schema": { "type": [ "null", "boolean" ], "default": null } },
      { "name": "data", "schema": { "default": null } },
      { "name": "map", "schema": { "$ref": "#/$defs/Twin.Map" } } ] }})"
};

const std::vector<std::string> TWIN_CALLS = {
  R"({})",
  R"({"target": {"movieid": 3}})",
  R"({"target": {"songid": 7}, "level": 2, "flag": true, "data": [1, "x"]})",
  R"({"target": {"movieid": 0}})",
  R"({"target": {"foo": 1}})",
  R"({"target": {"movieid": 3}, "item": {"a": "x", "b": true, "ref": 4}})",
  R"({"target": {"movieid": 3}, "item": {"b": true, "ref": 4}})",
  R"({"target": {"movieid": 3}, "item": {"a": "x", "ref": 4}})",
  R"({"target": {"movieid": 3}, "both": {"a": "x", "c": 1.5}})",
  R"({"target": {"movieid": 3}, "both": {"c": 1.5}})",
  R"({"target": {"movieid": 3}, "opt": {"a": "x"}})",
  R"({"target": {"movieid": 3}, "opt": {"shared": 2}})",
  R"({"target": {"movieid": 3}, "level": 0})",
  R"({"target": {"movieid": 3}, "filter": {}})",
  R"({"target": {"movieid": 3}, "filter": {"path": 7}})",
  R"({"target": {"movieid": 3}, "filter": {"path": "/x", "mode": "slow"}})",
  R"({"target": {"movieid": 3}, "filter": {"path": "/x", "mode": "other"}})",
  R"({"target": {"movieid": 3}, "filter": {"path": "/x", "extra": 1}})",
  R"({"target": {"movieid": 3}, "flag": 1})",
  R"({"target": {"movieid": 3}, "map": {"x": 1, "y": 2}})",
  R"({"target": {"movieid": 3}, "map": {"x": "one"}})",
  R"([{"movieid": 3}, null, null, null, 5])",
};
// clang-format on

} // unnamed namespace

class TestJSONServiceDescription : public ::testing::Test
{
protected:
  void SetUp() override { CJSONRPC::Cleanup(); }
  void TearDown() override { CJSONRPC::Cleanup(); }

  static bool AddTestMethod(const std::string& name, const std::string& params)
  {
    return CJSONServiceDescription::AddMethod(
        R"({")" + name +
            R"(": {"type": "method", "description": "test", "transport": "Response",
                   "permission": "ReadData", "returns": "string", "params": )" +
            params + "}}",
        StubMethod);
  }

  CVariant Introspect(bool descriptions = true)
  {
    CVariant result;
    EXPECT_EQ(OK, CJSONServiceDescription::Print(result, &m_transport, &m_client, descriptions,
                                                 true, false));
    return result;
  }

  JSONRPC_STATUS Call(const char* method, const std::string& paramsJson, CVariant& output)
  {
    std::string key = method;
    StringUtils::ToLower(key);
    MethodCall call;
    output = CVariant();
    return CJSONServiceDescription::CheckCall(key.c_str(), ParseJson(paramsJson), &m_transport,
                                              &m_client, false, call, output);
  }

  //! \brief Registers one form of the twins and records what Introspect and each call answer
  CVariant Record(bool draft2020)
  {
    CJSONRPC::Cleanup();
    for (const auto& twin : TWIN_TYPES)
      EXPECT_TRUE(CJSONServiceDescription::AddType(draft2020 ? twin.draft2020 : twin.draft03));
    EXPECT_TRUE(CJSONServiceDescription::AddMethod(
        draft2020 ? TWIN_METHOD.draft2020 : TWIN_METHOD.draft03, StubMethod));
    CJSONServiceDescription::ResolveReferences();

    CVariant record;
    record["introspect"] = Introspect();
    for (const auto& params : TWIN_CALLS)
    {
      CVariant output;
      CVariant answer;
      answer["status"] = static_cast<int>(Call("Twin.Method", params, output));
      answer["output"] = output;
      record["calls"][params] = answer;
    }
    return record;
  }

  CAllCapabilityTransport m_transport;
  CAllPermissionClient m_client;
};

TEST_F(TestJSONServiceDescription, A2020DefinitionReadsAsItsDraft03Twin)
{
  const CVariant draft03 = Record(false);
  const CVariant draft2020 = Record(true);

  ASSERT_EQ(TWIN_TYPES.size(), draft03["introspect"]["types"].size());
  ASSERT_TRUE(draft03["introspect"]["methods"].isMember("Twin.Method"));
  ExpectVariantEq(draft03["introspect"], draft2020["introspect"]);
  for (const auto& params : TWIN_CALLS)
  {
    SCOPED_TRACE(params);
    ExpectVariantEq(draft03["calls"][params], draft2020["calls"][params]);
  }

  // The calls reach both outcomes, so the comparison is not of two refusals
  EXPECT_EQ(OK, draft2020["calls"][R"({"target": {"movieid": 3}})"]["status"].asInteger());
  EXPECT_EQ(InvalidParams, draft2020["calls"][R"({"target": {"foo": 1}})"]["status"].asInteger());
}

TEST_F(TestJSONServiceDescription, RequiredArrayMatchesMixedCaseProperties)
{
  ASSERT_TRUE(AddTestMethod("Test.Case", R"([ { "name": "o", "required": true, "schema": {
      "type": "object",
      "properties": { "MixedCase": { "type": "string" } },
      "required": ["MixedCase"] } } ])"));
  ASSERT_TRUE(AddTestMethod("Test.CaseDraft03", R"([ { "name": "o", "required": true,
      "type": "object",
      "properties": { "MixedCase": { "type": "string", "required": true } } } ])"));

  for (const char* method : {"Test.Case", "Test.CaseDraft03"})
  {
    SCOPED_TRACE(method);
    CVariant output;
    EXPECT_EQ(OK, Call(method, R"({"o": {"MixedCase": "v"}})", output));
    EXPECT_EQ(InvalidParams, Call(method, R"({"o": {}})", output));
    EXPECT_EQ("MixedCase", output["stack"]["property"]["name"].asString());
    EXPECT_EQ("Missing property", output["stack"]["message"].asString());
  }
}

TEST_F(TestJSONServiceDescription, RequiredArrayNamingUnknownPropertyFailsParse)
{
  EXPECT_FALSE(AddTestMethod("Test.Bad", R"([ { "name": "o", "schema": {
      "type": "object",
      "properties": { "real": { "type": "string" } },
      "required": ["nonexistent"] } } ])"));
}

TEST_F(TestJSONServiceDescription, SchemaWithAMalformedTypeStillFailsParse)
{
  // Absent is not the same as nonsense: a "type" that is neither a name nor a list of names
  // is a broken schema and must not be read as "any".
  EXPECT_FALSE(AddTestMethod("Test.BadType", R"([ { "name": "data", "schema": { "type": 5 } } ])"));
}

TEST_F(TestJSONServiceDescription, AnAllOfMemberThatIsNotAReferenceFailsParse)
{
  EXPECT_FALSE(CJSONServiceDescription::AddType(
      R"({"Bad.Compose": { "allOf": [ { "type": "object" } ] }})"));
}

//! \brief Two types referencing each other resolve, whichever arrives first
TEST_F(TestJSONServiceDescription, MutuallyReferencingTypes)
{
  EXPECT_FALSE(CJSONServiceDescription::AddType(R"({"Cycle.A": {
    "type": "object",
    "properties": { "b": { "$ref": "#/$defs/Cycle.B" } }
  }})"));
  EXPECT_TRUE(CJSONServiceDescription::AddType(R"({"Cycle.B": {
    "type": "object",
    "properties": { "a": { "$ref": "#/$defs/Cycle.A" }, "n": { "type": "integer", "default": 3 } }
  }})"));
  CJSONServiceDescription::ResolveReferences();

  ASSERT_NE(nullptr, CJSONServiceDescription::GetType("Cycle.A"));
  ASSERT_TRUE(AddTestMethod(
      "Test.Cycle",
      R"([ { "name": "data", "required": true, "schema": { "$ref": "#/$defs/Cycle.A" } } ])"));

  // Both types validate: the default of the second fills in, and its constraint holds
  CVariant output;
  EXPECT_EQ(OK, Call("Test.Cycle", R"({"data": {"b": {"a": {}}}})", output));
  EXPECT_EQ(3, output["data"]["b"]["n"].asInteger());
  EXPECT_EQ(InvalidParams, Call("Test.Cycle", R"({"data": {"b": {"n": "x"}}})", output));
}

/*!
 A derived type registered before its base is replayed only once the base has parsed, so its
 own properties and defaults are enforced.
 */
TEST_F(TestJSONServiceDescription, ForwardCompositionReference)
{
  EXPECT_FALSE(CJSONServiceDescription::AddType(R"({"Late.Derived": {
    "allOf": [ { "$ref": "#/$defs/Late.Base" } ],
    "properties": {
      "b": { "type": "boolean" },
      "shared": { "type": "integer", "default": 2 }
    },
    "required": ["b"]
  }})"));
  EXPECT_TRUE(CJSONServiceDescription::AddType(R"({"Late.Base": {
    "type": "object",
    "properties": {
      "a": { "type": "string" },
      "shared": { "type": "integer", "default": 1 }
    },
    "required": ["a"]
  }})"));
  CJSONServiceDescription::ResolveReferences();

  ASSERT_NE(nullptr, CJSONServiceDescription::GetType("Late.Derived"));
  ASSERT_TRUE(AddTestMethod(
      "Test.LateExtends",
      R"([ { "name": "data", "required": true, "schema": { "$ref": "#/$defs/Late.Derived" } } ])"));

  CVariant output;
  EXPECT_EQ(OK, Call("Test.LateExtends", R"({"data": {"a": "x", "b": true}})", output));
  ExpectVariantEq(ParseJson(R"({ "data": { "a": "x", "b": true, "shared": 2 } })"), output);

  // The derived type's own required property is enforced
  EXPECT_EQ(InvalidParams, Call("Test.LateExtends", R"({"data": {"a": "x"}})", output));
}

/*!
 A single property can carry the annotation without the whole type being deprecated, and a
 deprecated reference does not deprecate the type it refers to.
 */
TEST_F(TestJSONServiceDescription, IntrospectReportsADeprecatedProperty)
{
  ASSERT_TRUE(CJSONServiceDescription::AddType(R"({"Dep.Target": { "type": "integer" }})"));
  ASSERT_TRUE(CJSONServiceDescription::AddType(R"({"Dep.Thing": {
    "type": "object",
    "properties": {
      "old": { "type": "integer", "deprecated": true, "description": "Use new instead" },
      "new": { "type": "integer" },
      "oldReference": { "$ref": "#/$defs/Dep.Target", "deprecated": true },
      "newReference": { "$ref": "#/$defs/Dep.Target" }
    }
  }})"));
  CJSONServiceDescription::ResolveReferences();

  for (const bool descriptions : {true, false})
  {
    SCOPED_TRACE(descriptions ? "with descriptions" : "without descriptions");
    const CVariant types{Introspect(descriptions)["types"]};
    const CVariant& properties{types["Dep.Thing"]["properties"]};

    ExpectVariantEq(CVariant(true), properties["old"]["deprecated"]);
    EXPECT_EQ(descriptions, properties["old"].isMember("description"));
    EXPECT_FALSE(properties["new"].isMember("deprecated"));
    ExpectVariantEq(CVariant(true), properties["oldreference"]["deprecated"]);
    EXPECT_FALSE(properties["newreference"].isMember("deprecated"));
    EXPECT_FALSE(types["Dep.Target"].isMember("deprecated"));
  }
}
