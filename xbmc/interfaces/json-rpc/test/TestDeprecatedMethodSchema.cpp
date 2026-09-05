/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"
#include "ServiceDescription.h"
#include "utils/JSONVariantParser.h"
#include "utils/Variant.h"

#include <algorithm>
#include <array>
#include <map>
#include <regex>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{

/*!
 \brief What a deprecated definition is superseded by

 A caller moved off the deprecated name has to land on something that exists,
 or the schema is telling them to break their client.
 */
struct Supersession
{
  const char* deprecated;
  const char* replacement;
};

//! \brief Deprecated names their replacement answers unchanged - a rename, nothing more
constexpr std::array<Supersession, 2> RENAMED_METHODS{{
    {"XBMC.GetInfoLabels", "GUI.GetInfoLabels"},
    {"XBMC.GetInfoBooleans", "GUI.GetInfoBooleans"},
}};

/*!
 \brief Deprecated methods a replacement covers under a different signature

 A caller has to rewrite the request rather than only the method name, so these
 are held apart from the renames above: what they share is that the old name
 still works and still names somewhere to go.
 */
constexpr std::array<Supersession, 4> SUPERSEDED_METHODS{{
    {"VideoLibrary.RefreshMovie", "VideoLibrary.Refresh"},
    {"VideoLibrary.RefreshTVShow", "VideoLibrary.Refresh"},
    {"VideoLibrary.RefreshEpisode", "VideoLibrary.Refresh"},
    {"VideoLibrary.RefreshMusicVideo", "VideoLibrary.Refresh"},
}};

//! \brief Every deprecated method, however its replacement is reached
std::vector<Supersession> DeprecatedMethods()
{
  std::vector<Supersession> methods{RENAMED_METHODS.begin(), RENAMED_METHODS.end()};
  methods.insert(methods.end(), SUPERSEDED_METHODS.begin(), SUPERSEDED_METHODS.end());
  return methods;
}

//! \brief Deprecated properties, as type name, property, and what replaces it
struct DeprecatedProperty
{
  const char* type;
  const char* property;
  const char* replacement;
};

constexpr std::array<DeprecatedProperty, 2> DEPRECATED_PROPERTIES{{
    {"PVR.Details.Broadcast", "seasonnum", "season"},
    {"PVR.Details.Broadcast", "episodenum", "episode"},
}};

//! \brief Every deprecated definition the schema declares, as "name" or "Type.property"
std::vector<std::string> DeclaredDeprecations()
{
  std::vector<std::string> found;

  for (const auto& [name, method] : ShippedMethods())
  {
    if (method["deprecated"].asBoolean(false))
      found.push_back(name);
  }

  for (const auto& [name, type] : ShippedTypes())
  {
    const CVariant& properties = type["properties"];
    for (auto property = properties.begin_map(); property != properties.end_map(); ++property)
    {
      if (property->second["deprecated"].asBoolean(false))
        found.push_back(name + "." + property->first);
    }
  }

  return found;
}

} // unnamed namespace

/*!
 Every deprecation the schema declares must be listed here, so the tests below
 cover the whole schema rather than whatever they happen to name.
 */
TEST(TestDeprecatedMethodSchema, EveryDeprecationIsAccountedFor)
{
  std::vector<std::string> expected;
  for (const auto& [deprecated, replacement] : DeprecatedMethods())
    expected.emplace_back(deprecated);
  for (const auto& [type, property, replacement] : DEPRECATED_PROPERTIES)
    expected.emplace_back(std::string(type) + "." + property);

  std::vector<std::string> declared{DeclaredDeprecations()};

  std::sort(expected.begin(), expected.end());
  std::sort(declared.begin(), declared.end());
  EXPECT_EQ(expected, declared);
}

TEST(TestDeprecatedMethodSchema, ADeprecatedMethodNamesAReplacementThatExists)
{
  const std::map<std::string, CVariant> methods{ShippedMethods()};

  for (const auto& [deprecated, replacement] : DeprecatedMethods())
  {
    ASSERT_TRUE(methods.contains(deprecated)) << deprecated;
    EXPECT_TRUE(methods.contains(replacement)) << replacement << " does not exist";

    const std::string description{methods.at(deprecated)["description"].asString()};
    EXPECT_NE(std::string::npos, description.find(replacement))
        << deprecated << " does not name " << replacement << " in its description";
  }
}

TEST(TestDeprecatedMethodSchema, ADeprecatedPropertyNamesAReplacementThatExists)
{
  const std::map<std::string, CVariant> types{ShippedTypes()};

  for (const auto& [typeName, property, replacement] : DEPRECATED_PROPERTIES)
  {
    ASSERT_TRUE(types.contains(typeName)) << typeName;
    const CVariant& properties = types.at(typeName)["properties"];

    ASSERT_TRUE(properties.isMember(property)) << property;
    EXPECT_TRUE(properties.isMember(replacement))
        << replacement << " does not exist on " << typeName;

    const std::string description{properties[property]["description"].asString()};
    EXPECT_NE(std::string::npos, description.find(replacement))
        << property << " does not name " << replacement << " in its description";
  }
}

/*!
 A rename is the same implementation under two names, so request and answer must
 agree; a method superseded under a different signature is exempt.
 */
TEST(TestDeprecatedMethodSchema, ARenamedMethodAgreesWithItsReplacement)
{
  const std::map<std::string, CVariant> methods{ShippedMethods()};

  for (const auto& [deprecated, replacement] : RENAMED_METHODS)
  {
    ASSERT_TRUE(methods.contains(deprecated)) << deprecated;
    ASSERT_TRUE(methods.contains(replacement)) << replacement;

    const CVariant& from{methods.at(deprecated)};
    const CVariant& to{methods.at(replacement)};

    EXPECT_TRUE(from["params"] == to["params"])
        << deprecated << " and " << replacement << " do not take the same parameters";
    EXPECT_TRUE(from["returns"] == to["returns"])
        << deprecated << " and " << replacement << " do not return the same thing";
    EXPECT_EQ(from["permission"].asString(), to["permission"].asString())
        << deprecated << " and " << replacement << " do not require the same permission";
  }
}

/*!
 The replacement is what callers are being sent to, so deprecating it as well
 would leave the schema pointing at a dead end.
 */
TEST(TestDeprecatedMethodSchema, AReplacementIsNotItselfDeprecated)
{
  const std::map<std::string, CVariant> methods{ShippedMethods()};
  const std::map<std::string, CVariant> types{ShippedTypes()};

  for (const auto& [deprecated, replacement] : DeprecatedMethods())
  {
    ASSERT_TRUE(methods.contains(replacement)) << replacement;
    EXPECT_FALSE(methods.at(replacement)["deprecated"].asBoolean(false)) << replacement;
  }

  for (const auto& [typeName, property, replacement] : DEPRECATED_PROPERTIES)
  {
    ASSERT_TRUE(types.contains(typeName)) << typeName;
    const CVariant& properties = types.at(typeName)["properties"];
    EXPECT_FALSE(properties[replacement]["deprecated"].asBoolean(false)) << replacement;
  }
}

/*!
 A removal schedule is revised between releases, so it lives in the API
 documentation rather than in the schema a client reads over the wire.
 */
TEST(TestDeprecatedMethodSchema, TheSchemaDoesNotDateItsOwnRemovals)
{
  const std::map<std::string, CVariant> methods{ShippedMethods()};

  for (const auto& [deprecated, replacement] : DeprecatedMethods())
  {
    const std::string description{methods.at(deprecated)["description"].asString()};
    EXPECT_EQ(std::string::npos, description.find("Kodi 2"))
        << deprecated << " names a Kodi version in its description";
    EXPECT_FALSE(std::regex_search(description, std::regex{"version [0-9]"}))
        << deprecated << " names an API version in its description";
  }
}
