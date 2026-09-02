/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/json-rpc/AddonsExecuteAddon.h"
#include "utils/ExecString.h"
#include "utils/Variant.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
constexpr const char* ADDON_ID = "script.toolbox";

CVariant MakeRequest(const CVariant& params)
{
  CVariant request(CVariant::VariantTypeObject);
  request["addonid"] = ADDON_ID;
  request["params"] = params;
  request["wait"] = false;

  return request;
}

CVariant ObjectParams(const std::string& key, const std::string& value)
{
  CVariant params(CVariant::VariantTypeObject);
  params[key] = value;

  return params;
}

/*! \brief The arguments the add-on actually receives.

 CBuiltins::Execute parses the command through CExecString, so what reaches the add-on is the
 result of that split rather than the command string itself.
 */
std::vector<std::string> ArgumentsSeenByTheAddon(const std::string& command)
{
  const CExecString exec(command);
  EXPECT_TRUE(exec.IsValid()) << command;

  return exec.GetParams();
}
} // unnamed namespace

TEST(TestExecuteAddonParams, NoParamsGivesTheBareBuiltin)
{
  CVariant request(CVariant::VariantTypeObject);
  request["addonid"] = ADDON_ID;
  request["params"] = "";
  request["wait"] = false;

  EXPECT_EQ("RunAddon(script.toolbox)", ParseExecuteAddonParams(request).command);
}

TEST(TestExecuteAddonParams, ObjectParamsBecomeKeyValueArguments)
{
  CVariant params(CVariant::VariantTypeObject);
  params["info"] = "builtin";
  params["id"] = "ReloadSkin()";

  const std::vector<std::string> argv =
      ArgumentsSeenByTheAddon(ParseExecuteAddonParams(MakeRequest(params)).command);

  ASSERT_EQ(3U, argv.size());
  EXPECT_EQ(ADDON_ID, argv[0]);
  // CVariant orders an object's members by key, so "id" precedes "info".
  EXPECT_EQ("id=ReloadSkin()", argv[1]);
  EXPECT_EQ("info=builtin", argv[2]);
}

TEST(TestExecuteAddonParams, ACommaInAnObjectValueStaysInOneArgument)
{
  const std::vector<std::string> argv = ArgumentsSeenByTheAddon(
      ParseExecuteAddonParams(MakeRequest(ObjectParams("title", "Peaches, Herb"))).command);

  ASSERT_EQ(2U, argv.size());
  EXPECT_EQ(ADDON_ID, argv[0]);
  EXPECT_EQ("title=Peaches, Herb", argv[1]);
}

TEST(TestExecuteAddonParams, AQuoteInAnObjectValueStaysInOneArgument)
{
  const std::vector<std::string> argv = ArgumentsSeenByTheAddon(
      ParseExecuteAddonParams(MakeRequest(ObjectParams("quote", "say \"hi\", then go"))).command);

  ASSERT_EQ(2U, argv.size());
  EXPECT_EQ("quote=say \"hi\", then go", argv[1]);
}

TEST(TestExecuteAddonParams, ACommaInAnArrayValueStaysInOneArgument)
{
  CVariant params(CVariant::VariantTypeArray);
  params.push_back("Peaches, Herb");

  const std::vector<std::string> argv =
      ArgumentsSeenByTheAddon(ParseExecuteAddonParams(MakeRequest(params)).command);

  ASSERT_EQ(2U, argv.size());
  EXPECT_EQ("Peaches, Herb", argv[1]);
}

TEST(TestExecuteAddonParams, WaitIsReadFromTheDeclaredTopLevelParameter)
{
  CVariant request = MakeRequest(ObjectParams("info", "builtin"));
  request["wait"] = true;

  EXPECT_TRUE(ParseExecuteAddonParams(request).wait);
}

TEST(TestExecuteAddonParams, WaitDefaultsToNotWaiting)
{
  EXPECT_FALSE(ParseExecuteAddonParams(MakeRequest(ObjectParams("info", "builtin"))).wait);
}

TEST(TestExecuteAddonParams, AWaitKeyInsideParamsIsJustAnAddonArgument)
{
  const ParsedExecuteAddon parsed =
      ParseExecuteAddonParams(MakeRequest(ObjectParams("wait", "true")));

  EXPECT_FALSE(parsed.wait);

  const std::vector<std::string> argv = ArgumentsSeenByTheAddon(parsed.command);
  ASSERT_EQ(2U, argv.size());
  EXPECT_EQ("wait=true", argv[1]);
}
