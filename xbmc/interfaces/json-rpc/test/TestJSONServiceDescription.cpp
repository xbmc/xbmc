/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"

#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{

void ExpectVariantEq(const CVariant& expected, const CVariant& actual)
{
  EXPECT_TRUE(expected == actual) << "expected: " << ToJson(expected)
                                  << "\n  actual: " << ToJson(actual);
}

} // unnamed namespace

class TestJSONServiceDescription : public JSONServiceDescriptionTestBase
{
};

TEST_F(TestJSONServiceDescription, MissingRequiredParameter)
{
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Required": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [ { "name": "value", "required": true, "schema": { "type": "string" } } ],
    "returns": "string"
  }})",
                                                 StubMethod));

  CVariant output;
  EXPECT_EQ(InvalidParams, Call("Test.Required", "{}", output));
  ExpectVariantEq(ParseJson(R"({
    "method": "Test.Required",
    "stack": { "name": "value", "type": "string", "message": "Missing parameter" }
  })"),
                  output);
}

TEST_F(TestJSONServiceDescription, ObjectParameter)
{
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Object": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [
      { "name": "opts", "required": true, "schema": {
          "type": "object",
          "properties": {
            "path": { "type": "string" },
            "mode": { "type": "string", "default": "fast" }
          },
          "required": ["path"],
          "additionalProperties": false } },
      { "name": "speed", "schema": { "type": "integer", "default": 5 } }
    ],
    "returns": "string"
  }})",
                                                 StubMethod));

  // A missing required property names the property and its type in the error data
  CVariant output;
  EXPECT_EQ(InvalidParams, Call("Test.Object", R"({"opts": {}})", output));
  ExpectVariantEq(ParseJson(R"({
    "method": "Test.Object",
    "stack": {
      "name": "opts",
      "type": "object",
      "property": { "name": "path", "type": "string" },
      "message": "Missing property"
    }
  })"),
                  output);

  // A property of the wrong type carries the message on the property frame
  EXPECT_EQ(InvalidParams, Call("Test.Object", R"({"opts": {"path": 7}})", output));
  ExpectVariantEq(ParseJson(R"({
    "method": "Test.Object",
    "stack": {
      "name": "opts",
      "type": "object",
      "property": { "name": "path", "type": "string", "message": "Invalid type integer received" }
    }
  })"),
                  output);

  // Optional properties and parameters are filled from their defaults
  EXPECT_EQ(OK, Call("Test.Object", R"({"opts": {"path": "/x"}})", output));
  ExpectVariantEq(ParseJson(R"({ "opts": { "mode": "fast", "path": "/x" }, "speed": 5 })"), output);
}

TEST_F(TestJSONServiceDescription, UnionParameter)
{
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Union": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [
      { "name": "target", "required": true, "schema": { "anyOf": [
          { "type": "object",
            "properties": { "movieid": { "type": "integer" } },
            "required": ["movieid"],
            "additionalProperties": false },
          { "type": "object",
            "properties": { "songid": { "type": "integer" } },
            "required": ["songid"],
            "additionalProperties": false }
        ] } },
      { "name": "when", "schema": { "anyOf": [
          { "type": "null" },
          { "type": "string", "enum": ["now", "later"] }
        ], "default": null } },
      { "name": "flag", "schema": { "type": ["null", "boolean"], "default": null } }
    ],
    "returns": "string"
  }})",
                                                 StubMethod));

  CVariant output;
  EXPECT_EQ(OK, Call("Test.Union", R"({"target": {"movieid": 3}})", output));
  ExpectVariantEq(ParseJson(R"({ "target": { "movieid": 3 }, "when": null, "flag": null })"),
                  output);

  EXPECT_EQ(OK, Call("Test.Union", R"({"target": {"songid": 7}, "when": "later", "flag": true})",
                     output));
  ExpectVariantEq(ParseJson(R"({ "target": { "songid": 7 }, "when": "later", "flag": true })"),
                  output);

  // No union branch accepts the value: the error carries the OR'd type list
  EXPECT_EQ(InvalidParams, Call("Test.Union", R"({"target": {"foo": 1}})", output));
  ExpectVariantEq(ParseJson(R"({
    "method": "Test.Union",
    "stack": {
      "name": "target",
      "type": "object",
      "message": "Received value does not match any of the union type definitions"
    }
  })"),
                  output);

  // A value matching the type mask but no branch constraint fails the union check
  EXPECT_EQ(InvalidParams,
            Call("Test.Union", R"({"target": {"movieid": 3}, "when": "never"})", output));
  ExpectVariantEq(ParseJson(R"({
    "method": "Test.Union",
    "stack": {
      "name": "when",
      "type": ["null", "string"],
      "message": "Received value does not match any of the union type definitions"
    }
  })"),
                  output);

  // A value outside the OR'd type mask fails before any branch is tried
  EXPECT_EQ(InvalidParams, Call("Test.Union", R"({"target": {"movieid": 3}, "when": 5})", output));
  ExpectVariantEq(ParseJson(R"({
    "method": "Test.Union",
    "stack": {
      "name": "when",
      "type": ["null", "string"],
      "message": "Invalid type integer received"
    }
  })"),
                  output);

  // Pure string unions behave identically
  EXPECT_EQ(InvalidParams, Call("Test.Union", R"({"target": {"movieid": 3}, "flag": 1})", output));
  ExpectVariantEq(ParseJson(R"({
    "method": "Test.Union",
    "stack": {
      "name": "flag",
      "type": ["null", "boolean"],
      "message": "Invalid type integer received"
    }
  })"),
                  output);
}

TEST_F(TestJSONServiceDescription, ExtendedType)
{
  ASSERT_TRUE(CJSONServiceDescription::AddType(R"({"Base.A": {
    "type": "object",
    "properties": {
      "a": { "type": "string" },
      "shared": { "type": "integer", "default": 1 }
    },
    "required": ["a"]
  }})"));
  ASSERT_TRUE(CJSONServiceDescription::AddType(R"({"Derived.B": {
    "allOf": [ { "$ref": "#/$defs/Base.A" } ],
    "properties": {
      "b": { "type": "boolean" },
      "shared": { "type": "integer", "default": 2 }
    },
    "required": ["b"]
  }})"));
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Extends": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [ { "name": "data", "required": true, "schema": { "$ref": "#/$defs/Derived.B" } } ],
    "returns": "string"
  }})",
                                                 StubMethod));
  CJSONServiceDescription::ResolveReferences();

  // The base type is validated first and fills the output first; the derived
  // type's defaults overwrite the base's for same-named optional properties
  CVariant output;
  EXPECT_EQ(OK, Call("Test.Extends", R"({"data": {"a": "x", "b": true}})", output));
  ExpectVariantEq(ParseJson(R"({ "data": { "a": "x", "b": true, "shared": 2 } })"), output);

  // A value failing the base type reports the extended type by name
  EXPECT_EQ(InvalidParams, Call("Test.Extends", R"({"data": {"b": true}})", output));
  ExpectVariantEq(ParseJson(R"({
    "method": "Test.Extends",
    "stack": {
      "name": "Base.A",
      "type": "object",
      "property": { "name": "a", "type": "string" },
      "message": "value does not match extended type Base.A"
    }
  })"),
                  output);
}

TEST_F(TestJSONServiceDescription, ForwardReferences)
{
  // A reference to a type that arrives later parks until the base has parsed; methods are
  // registered only after all types, as at startup.
  EXPECT_FALSE(CJSONServiceDescription::AddType(R"({"C.Container": {
    "type": "object",
    "properties": { "inner": { "$ref": "#/$defs/C.Base" } },
    "required": ["inner"]
  }})"));
  EXPECT_TRUE(CJSONServiceDescription::AddType(R"({"C.Base": {
    "type": "object",
    "properties": {
      "x": { "type": "integer" },
      "y": { "type": "integer", "default": 9 }
    },
    "required": ["x"]
  }})"));
  CJSONServiceDescription::ResolveReferences();

  EXPECT_NE(nullptr, CJSONServiceDescription::GetType("C.Container"));
  EXPECT_NE(nullptr, CJSONServiceDescription::GetType("C.Base"));
  EXPECT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Forward": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [ { "name": "data", "required": true, "schema": { "$ref": "#/$defs/C.Container" } } ],
    "returns": "string"
  }})",
                                                 StubMethod));

  CVariant output;
  EXPECT_EQ(OK, Call("Test.Forward", R"({"data": {"inner": {"x": 1}}})", output));
  ExpectVariantEq(ParseJson(R"({ "data": { "inner": { "x": 1, "y": 9 } } })"), output);
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
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Cycle": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [ { "name": "data", "required": true, "schema": { "$ref": "#/$defs/Cycle.A" } } ],
    "returns": "string"
  }})",
                                                 StubMethod));

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
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.LateExtends": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [ { "name": "data", "required": true, "schema": { "$ref": "#/$defs/Late.Derived" } } ],
    "returns": "string"
  }})",
                                                 StubMethod));

  CVariant output;
  EXPECT_EQ(OK, Call("Test.LateExtends", R"({"data": {"a": "x", "b": true}})", output));
  ExpectVariantEq(ParseJson(R"({ "data": { "a": "x", "b": true, "shared": 2 } })"), output);

  // The derived type's own required property is enforced
  EXPECT_EQ(InvalidParams, Call("Test.LateExtends", R"({"data": {"a": "x"}})", output));
}

TEST_F(TestJSONServiceDescription, ReferenceWithLocalDefault)
{
  ASSERT_TRUE(CJSONServiceDescription::AddType(R"({"Level.T": {
    "type": "integer", "minimum": 0, "maximum": 10, "default": 5
  }})"));
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Ref": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [ { "name": "level", "schema": { "$ref": "#/$defs/Level.T", "default": 7 } } ],
    "returns": "string"
  }})",
                                                 StubMethod));
  CJSONServiceDescription::ResolveReferences();

  // The parameter's own default overrides the referenced type's default
  CVariant output;
  EXPECT_EQ(OK, Call("Test.Ref", "{}", output));
  ExpectVariantEq(ParseJson(R"({ "level": 7 })"), output);

  // Constraints of the referenced type still apply, message text included
  EXPECT_EQ(InvalidParams, Call("Test.Ref", R"({"level": 12})", output));
  ExpectVariantEq(ParseJson(R"({
    "method": "Test.Ref",
    "stack": {
      "name": "level",
      "type": "integer",
      "message": "Value between 0 (inclusive) and 10 (inclusive) expected but 12 received"
    }
  })"),
                  output);

  // Resolving references again must not change behaviour
  CJSONServiceDescription::ResolveReferences();
  CJSONServiceDescription::ResolveReferences();
  EXPECT_EQ(OK, Call("Test.Ref", "{}", output));
  ExpectVariantEq(ParseJson(R"({ "level": 7 })"), output);
}

TEST_F(TestJSONServiceDescription, EnumParameter)
{
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Enum": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [ { "name": "mode", "schema": { "type": "string", "enum": ["one", "two"] } } ],
    "returns": "string"
  }})",
                                                 StubMethod));

  // An enum type without an explicit default defaults to its first value
  CVariant output;
  EXPECT_EQ(OK, Call("Test.Enum", "{}", output));
  ExpectVariantEq(ParseJson(R"({ "mode": "one" })"), output);

  EXPECT_EQ(InvalidParams, Call("Test.Enum", R"({"mode": "three"})", output));
  ExpectVariantEq(ParseJson(R"({
    "method": "Test.Enum",
    "stack": {
      "name": "mode",
      "type": "string",
      "message": "Received value does not match any of the defined enum values"
    }
  })"),
                  output);
}

TEST_F(TestJSONServiceDescription, RequiredArrayMatchesMixedCaseProperties)
{
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Case": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [ { "name": "o", "required": true, "schema": {
      "type": "object",
      "properties": { "MixedCase": { "type": "string" } },
      "required": ["MixedCase"] } } ],
    "returns": "string"
  }})",
                                                 StubMethod));

  CVariant output;
  EXPECT_EQ(OK, Call("Test.Case", R"({"o": {"MixedCase": "v"}})", output));
  ExpectVariantEq(ParseJson(R"({ "o": { "MixedCase": "v" } })"), output);

  EXPECT_EQ(InvalidParams, Call("Test.Case", R"({"o": {}})", output));
  ExpectVariantEq(ParseJson(R"({
    "method": "Test.Case",
    "stack": {
      "name": "o",
      "type": "object",
      "property": { "name": "MixedCase", "type": "string" },
      "message": "Missing property"
    }
  })"),
                  output);
}

TEST_F(TestJSONServiceDescription, RequiredArrayNamingUnknownPropertyFailsParse)
{
  EXPECT_FALSE(CJSONServiceDescription::AddMethod(R"({"Test.Bad": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [ { "name": "o", "schema": {
      "type": "object",
      "properties": { "real": { "type": "string" } },
      "required": ["nonexistent"] } } ],
    "returns": "string"
  }})",
                                                  StubMethod));
}

TEST_F(TestJSONServiceDescription, SchemaWithoutATypeAcceptsAnyValue)
{
  // An omitted "type" constrains nothing, which is how the schema spells a value of any
  // type - so leaving the keyword out has to parse rather than fail.
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Any": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [ { "name": "data", "schema": { "default": null } } ],
    "returns": {}
  }})",
                                                 StubMethod));

  CVariant output;
  EXPECT_EQ(OK, Call("Test.Any", R"({"data": "a string"})", output));
  ExpectVariantEq(ParseJson(R"({ "data": "a string" })"), output);

  EXPECT_EQ(OK, Call("Test.Any", R"({"data": 42})", output));
  ExpectVariantEq(ParseJson(R"({ "data": 42 })"), output);

  EXPECT_EQ(OK, Call("Test.Any", R"({"data": {"nested": [1, 2]}})", output));
  ExpectVariantEq(ParseJson(R"({ "data": { "nested": [1, 2] } })"), output);

  // Omitting it altogether falls back to the declared default
  EXPECT_EQ(OK, Call("Test.Any", "{}", output));
  ExpectVariantEq(ParseJson(R"({ "data": null })"), output);
}

TEST_F(TestJSONServiceDescription, SchemaWithAMalformedTypeStillFailsParse)
{
  // Absent is not the same as nonsense: a "type" that is neither a name nor a list of names
  // is a broken schema and must not be read as "any".
  EXPECT_FALSE(CJSONServiceDescription::AddMethod(R"({"Test.BadType": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [ { "name": "data", "schema": { "type": 5 } } ],
    "returns": "string"
  }})",
                                                  StubMethod));
}

TEST_F(TestJSONServiceDescription, PrintEmits2020Dialect)
{
  ASSERT_TRUE(CJSONServiceDescription::AddType(
      R"({"Print.Enum": { "type": "string", "enum": ["a", "b"] }})"));
  ASSERT_TRUE(CJSONServiceDescription::AddType(
      R"({"Print.Base": {
        "type": "object",
        "properties": { "p": { "type": "string" } },
        "required": ["p"]
      }})"));
  ASSERT_TRUE(CJSONServiceDescription::AddType(
      R"({"Print.Derived": {
        "allOf": [ { "$ref": "#/$defs/Print.Base" } ],
        "properties": { "q": { "type": "integer" } }
      }})"));
  ASSERT_TRUE(CJSONServiceDescription::AddType(
      R"({"Print.Union": { "anyOf": [
        { "type": "string" },
        { "type": "integer" }
      ] }})"));
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Print.Method": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [ { "name": "target", "required": true, "description": "what to print",
                  "schema": { "$ref": "#/$defs/Print.Enum" } } ],
    "returns": "string"
  }})",
                                                 StubMethod));
  CJSONServiceDescription::ResolveReferences();

  CVariant result;
  ASSERT_EQ(OK, CJSONServiceDescription::Print(result, &m_transport, &m_client, true, true, false));

  const CVariant& types = result["types"];
  ASSERT_TRUE(types.isMember("Print.Enum"));

  ExpectVariantEq(ParseJson(R"(["a", "b"])"), types["Print.Enum"]["enum"]);
  EXPECT_FALSE(types["Print.Enum"].isMember("enums"));

  ExpectVariantEq(ParseJson(R"([ { "$ref": "#/$defs/Print.Base" } ])"),
                  types["Print.Derived"]["allOf"]);
  EXPECT_FALSE(types["Print.Derived"].isMember("extends"));

  ASSERT_TRUE(types["Print.Union"]["anyOf"].isArray());
  EXPECT_EQ(2U, types["Print.Union"]["anyOf"].size());
  EXPECT_FALSE(types["Print.Union"].isMember("type"));

  // Requiredness of properties is an array on the object, not a boolean
  ExpectVariantEq(ParseJson(R"(["p"])"), types["Print.Base"]["required"]);
  EXPECT_FALSE(types["Print.Base"]["properties"]["p"].isMember("required"));
  ExpectVariantEq(CVariant("Print.Base"), types["Print.Base"]["id"]);

  // Parameters are printed as content descriptors
  const CVariant& param = result["methods"]["Print.Method"]["params"][0];
  ExpectVariantEq(ParseJson(R"({
    "name": "target",
    "required": true,
    "description": "what to print",
    "schema": { "$ref": "#/$defs/Print.Enum" }
  })"),
                  param);
}

TEST_F(TestJSONServiceDescription, IntrospectReportsADeprecatedMethod)
{
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Old": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "deprecated": true,
    "params": [],
    "returns": "string"
  }})",
                                                 StubMethod));
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.New": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [],
    "returns": "string"
  }})",
                                                 StubMethod));

  CVariant result;
  ASSERT_EQ(OK, CJSONServiceDescription::Print(result, &m_transport, &m_client, true, true, false));

  ExpectVariantEq(CVariant(true), result["methods"]["Test.Old"]["deprecated"]);
  EXPECT_FALSE(result["methods"]["Test.New"].isMember("deprecated"));
}

//! \brief Suppressing descriptions must not also drop the deprecation annotation
TEST_F(TestJSONServiceDescription, DeprecationSurvivesDescriptionsBeingSuppressed)
{
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Old": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "deprecated": true,
    "params": [],
    "returns": "string"
  }})",
                                                 StubMethod));

  CVariant result;
  ASSERT_EQ(OK,
            CJSONServiceDescription::Print(result, &m_transport, &m_client, false, true, false));

  EXPECT_FALSE(result["methods"]["Test.Old"].isMember("description"));
  ExpectVariantEq(CVariant(true), result["methods"]["Test.Old"]["deprecated"]);
}

//! \brief A single property can carry the annotation without the whole type being deprecated
TEST_F(TestJSONServiceDescription, IntrospectReportsADeprecatedProperty)
{
  ASSERT_TRUE(CJSONServiceDescription::AddType(R"({"Dep.Thing": {
    "type": "object",
    "properties": {
      "old": { "type": "integer", "deprecated": true, "description": "Use new instead" },
      "new": { "type": "integer" }
    }
  }})"));
  CJSONServiceDescription::ResolveReferences();

  CVariant result;
  ASSERT_EQ(OK, CJSONServiceDescription::Print(result, &m_transport, &m_client, true, true, false));

  const CVariant& properties = result["types"]["Dep.Thing"]["properties"];
  ExpectVariantEq(CVariant(true), properties["old"]["deprecated"]);
  EXPECT_FALSE(properties["new"].isMember("deprecated"));
}

TEST_F(TestJSONServiceDescription, ADeprecatedReferenceKeepsItsAnnotation)
{
  ASSERT_TRUE(CJSONServiceDescription::AddType(R"({"Dep.Target": { "type": "integer" }})"));
  ASSERT_TRUE(CJSONServiceDescription::AddType(R"({"Dep.Holder": {
    "type": "object",
    "properties": {
      "old": { "$ref": "#/$defs/Dep.Target", "deprecated": true },
      "new": { "$ref": "#/$defs/Dep.Target" }
    }
  }})"));
  CJSONServiceDescription::ResolveReferences();

  CVariant result;
  ASSERT_EQ(OK, CJSONServiceDescription::Print(result, &m_transport, &m_client, true, true, false));

  const CVariant& properties = result["types"]["Dep.Holder"]["properties"];
  ExpectVariantEq(CVariant(true), properties["old"]["deprecated"]);
  EXPECT_FALSE(properties["new"].isMember("deprecated"));
}

//! \brief The annotation is part of the contract, so it outlives suppressed descriptions
TEST_F(TestJSONServiceDescription, ADeprecatedPropertySurvivesDescriptionsBeingSuppressed)
{
  ASSERT_TRUE(CJSONServiceDescription::AddType(R"({"Dep.Thing": {
    "type": "object",
    "properties": {
      "old": { "type": "integer", "deprecated": true, "description": "Use new instead" }
    }
  }})"));
  CJSONServiceDescription::ResolveReferences();

  CVariant result;
  ASSERT_EQ(OK,
            CJSONServiceDescription::Print(result, &m_transport, &m_client, false, true, false));

  const CVariant& property = result["types"]["Dep.Thing"]["properties"]["old"];
  ExpectVariantEq(CVariant(true), property["deprecated"]);
  EXPECT_FALSE(property.isMember("description"));
}
