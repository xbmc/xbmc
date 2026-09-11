/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"
#include "ServiceDescription.h"

#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace JSONRPC;

class TestJSONServiceDescriptionIntrospect : public JSONServiceDescriptionTestBase
{
};

/*!
 A definition a parser gate rejects vanishes silently, so the counts of what
 reaches the Introspect output are exact.
 */
TEST_F(TestJSONServiceDescriptionIntrospect, EveryDefinitionSurvivesToIntrospect)
{
  AddShippedServiceDescription();

  const size_t typeCount = std::size(JSONRPC_SERVICE_TYPES);
  const size_t methodCount = std::size(JSONRPC_SERVICE_METHODS);
  const size_t notificationCount = std::size(JSONRPC_SERVICE_NOTIFICATIONS);

  CVariant result;
  ASSERT_EQ(OK, CJSONServiceDescription::Print(result, &m_transport, &m_client, true, true, false));

  EXPECT_EQ(typeCount + RuntimeEnumNames().size(), result["types"].size());
  EXPECT_EQ(methodCount, result["methods"].size());
  EXPECT_EQ(notificationCount, result["notifications"].size());
  EXPECT_EQ(JSONRPC_STATUS_DESCRIPTIONS.size(), result["errors"].size());

  for (auto method = result["methods"].begin_map(); method != result["methods"].end_map(); ++method)
  {
    EXPECT_TRUE(method->second["errors"].isArray()) << method->first << " declares no errors";
  }
}

//! \brief The service header identifies the API as Kodi, not XBMC
TEST_F(TestJSONServiceDescriptionIntrospect, TheServiceHeaderIsNotBranded)
{
  CVariant result;
  ASSERT_EQ(OK, CJSONServiceDescription::Print(result, &m_transport, &m_client, true, true, false));

  EXPECT_EQ(std::string::npos, result["id"].asString().find("xbmc"))
      << "service id: " << result["id"].asString();
  EXPECT_EQ(std::string::npos, result["description"].asString().find("XBMC"))
      << "service description: " << result["description"].asString();
}

//! \brief A method's declared errors are served under it, with their descriptions
TEST_F(TestJSONServiceDescriptionIntrospect, DeclaredErrorsAreServedWithTheMethod)
{
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.Errors": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [], "returns": "string", "errors": ["NotFound", "Unavailable"]
  }})",
                                                 StubMethod));

  CVariant result;
  ASSERT_EQ(OK, CJSONServiceDescription::Print(result, &m_transport, &m_client, true, true, false,
                                               "Test.Errors", "method"));

  const CVariant& errors = result["methods"]["Test.Errors"]["errors"];
  ASSERT_EQ(2u, errors.size());
  EXPECT_EQ("NotFound", errors[0].asString());
  EXPECT_EQ("Unavailable", errors[1].asString());

  EXPECT_EQ(2u, result["errors"].size());
  EXPECT_TRUE(result["errors"].isMember("NotFound"));
  EXPECT_TRUE(result["errors"].isMember("Unavailable"));
}

TEST_F(TestJSONServiceDescriptionIntrospect, AMethodDeclaringNoErrorsServesAnEmptyList)
{
  ASSERT_TRUE(CJSONServiceDescription::AddMethod(R"({"Test.NoErrors": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [], "returns": "string", "errors": []
  }})",
                                                 StubMethod));

  CVariant result;
  ASSERT_EQ(OK, CJSONServiceDescription::Print(result, &m_transport, &m_client, true, true, false,
                                               "Test.NoErrors", "method"));

  const CVariant& errors = result["methods"]["Test.NoErrors"]["errors"];
  EXPECT_TRUE(errors.isArray());
  EXPECT_EQ(0u, errors.size());
  EXPECT_EQ(0u, result["errors"].size());
}

TEST_F(TestJSONServiceDescriptionIntrospect, AnUnknownErrorNameIsRejected)
{
  EXPECT_FALSE(CJSONServiceDescription::AddMethod(R"({"Test.Unknown": {
    "type": "method", "description": "test", "transport": "Response", "permission": "ReadData",
    "params": [], "returns": "string", "errors": ["NoSuchError"]
  }})",
                                                  StubMethod));
}
