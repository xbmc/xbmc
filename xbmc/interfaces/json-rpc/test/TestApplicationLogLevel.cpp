/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"
#include "ServiceBroker.h"
#include "ServiceDescription.h"
#include "commons/ilog.h"
#include "interfaces/json-rpc/ApplicationOperations.h"
#include "interfaces/json-rpc/JSONRPCUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <map>
#include <optional>
#include <set>
#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{

class CTestApplicationOperations : public CApplicationOperations
{
public:
  static std::string Name(int level) { return LogLevelName(level); }
  static std::optional<int> FromName(const std::string& name) { return LogLevelFromName(name); }
};

/*!
 \brief Puts the log level back when a test leaves scope, so the suite's own logging is unaffected
 */
class CLogLevelGuard
{
public:
  CLogLevelGuard()
    : m_level{CServiceBroker::GetLogging().GetLogLevel()},
      m_advancedLevel{CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel},
      m_toggle{CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_DEBUG_SHOWLOGINFO)}
  {
  }
  ~CLogLevelGuard()
  {
    // the toggle first: its callback moves the level, and the level is restored after it
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
        CSettings::SETTING_DEBUG_SHOWLOGINFO, m_toggle);
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel = m_advancedLevel;
    CServiceBroker::GetLogging().SetLogLevel(m_level);
  }

private:
  int m_level;
  int m_advancedLevel;
  bool m_toggle;
};

} // unnamed namespace

/*!
 The level is reported as a property of the application, next to volume and mute, and
 moved by a setter next to theirs.
 */
TEST(TestApplicationLogLevel, TheLevelIsAnApplicationProperty)
{
  EXPECT_TRUE(EnumValues(ShippedType("Application.Property.Name")).contains("loglevel"));
  EXPECT_EQ(ShippedType("Application.Property.Value")["properties"]["loglevel"]["$ref"].asString(),
            "#/$defs/Application.LogLevel.Value");
}

/*!
 Both parameters are optional and default to null, because the service description fills
 every omitted parameter before the handler runs: null is the only value that can mean
 "leave it as it is".
 */
TEST(TestApplicationLogLevel, TheSetterLeavesWhatItIsNotGiven)
{
  const CVariant method{ShippedMethod("Application.SetLogLevel")};
  EXPECT_EQ(method["permission"].asString(), "ControlSystem");

  const std::map<std::string, CVariant> params{Params(method)};
  ASSERT_TRUE(params.contains("level"));
  ASSERT_TRUE(params.contains("components"));
  EXPECT_FALSE(params.at("level")["required"].asBoolean());
  EXPECT_FALSE(params.at("components")["required"].asBoolean());
  EXPECT_TRUE(params.at("level")["schema"]["default"].isNull());
  EXPECT_TRUE(params.at("components")["schema"]["default"].isNull());
  EXPECT_EQ(method["returns"]["$ref"].asString(), "#/$defs/Application.LogLevel.Value");
}

/*!
 Every name the schema offers is one the handler understands, and every level the handler
 can report is one the schema declares - so the two cannot drift apart silently.
 */
TEST(TestApplicationLogLevel, TheSchemaAndTheHandlerAgreeOnTheNames)
{
  const std::set<std::string> names{EnumValues(ShippedType("Application.LogLevel"))};
  ASSERT_EQ(names.size(), 4u);

  for (const std::string& name : names)
  {
    const std::optional<int> level{CTestApplicationOperations::FromName(name)};
    ASSERT_TRUE(level) << name << " is in the schema and unknown to the handler";
    EXPECT_EQ(CTestApplicationOperations::Name(*level), name);
  }

  for (int level = LOG_LEVEL_NONE; level <= LOG_LEVEL_MAX; ++level)
  {
    EXPECT_TRUE(names.contains(CTestApplicationOperations::Name(level)))
        << "level " << level << " has no name in the schema";
  }

  EXPECT_TRUE(CTestApplicationOperations::Name(LOG_LEVEL_MAX + 1).empty());
  EXPECT_FALSE(CTestApplicationOperations::FromName("verbose"));
}

TEST(TestApplicationLogLevel, AnUnknownComponentIsRejected)
{
  CVariant params{CVariant::VariantTypeObject};
  params["level"] = CVariant{};
  params["components"] = CVariant{CVariant::VariantTypeArray};
  params["components"].append("jsonrpc");
  params["components"].append("no-such-component");

  CVariant result;
  EXPECT_EQ(CApplicationOperations::SetLogLevel("", nullptr, nullptr, params, result),
            InvalidParams);
}

/*!
 The answer is what is now in force, read back from the logger rather than echoed from
 the request.
 */
TEST(TestApplicationLogLevel, TheSetterAnswersWithWhatIsInForce)
{
  CLogLevelGuard guard;

  CVariant params{CVariant::VariantTypeObject};
  params["level"] = "normal";
  params["components"] = CVariant{};

  CVariant result;
  ASSERT_EQ(CApplicationOperations::SetLogLevel("", nullptr, nullptr, params, result), OK);
  EXPECT_EQ(result["level"].asString(), "normal");
  EXPECT_EQ(CServiceBroker::GetLogging().GetLogLevel(), LOG_LEVEL_NORMAL);

  params["level"] = "debug";
  ASSERT_EQ(CApplicationOperations::SetLogLevel("", nullptr, nullptr, params, result), OK);
  EXPECT_EQ(result["level"].asString(), "debug");
  EXPECT_EQ(CServiceBroker::GetLogging().GetLogLevel(), LOG_LEVEL_DEBUG);

  // every component this build knows is listed, by its stable name
  std::set<std::string> listed;
  for (auto it = result["components"].begin_array(); it != result["components"].end_array(); ++it)
  {
    listed.insert((*it)["name"].asString());
    EXPECT_TRUE((*it)["enabled"].isBoolean());
  }
  for (const std::string& name : CLog::GetComponentNames())
    EXPECT_TRUE(listed.contains(name)) << name;
}
