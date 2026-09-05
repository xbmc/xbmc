/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"
#include "interfaces/json-rpc/JSONRPCUtils.h"
#include "utils/Variant.h"

#include <array>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
constexpr std::array<OperationPermission, 14> PERMISSIONS{
    ReadData,    ControlPlayback, ControlNotify, ControlPower,  UpdateData,
    RemoveData,  Navigate,        WriteFile,     ControlSystem, ControlGUI,
    ManageAddon, ExecuteAddon,    ControlPVR,    WriteSetting};
} // namespace

TEST(TestJSONRPCPermission, EveryPermissionHasAName)
{
  for (const auto permission : PERMISSIONS)
    EXPECT_STRNE("Unknown", PermissionToString(permission)) << "value " << permission;
}

TEST(TestJSONRPCPermission, NameRoundTripsBackToTheSamePermission)
{
  for (const auto permission : PERMISSIONS)
  {
    const char* const name = PermissionToString(permission);
    EXPECT_EQ(permission, StringToPermission(name)) << name;
  }
}

//! \brief A name that is not a permission must not read as one, or a misspelled permission in
//! a schema would silently gate its method at whatever the fallback was
TEST(TestJSONRPCPermission, UnrecognisedNameIsRefused)
{
  EXPECT_FALSE(StringToPermission("NoSuchPermission").has_value());
  EXPECT_FALSE(StringToPermission("").has_value());
  EXPECT_FALSE(StringToPermission("Unknown").has_value());
  EXPECT_FALSE(StringToPermission("controlpvr").has_value()) << "matching is case sensitive";
}

TEST(TestJSONRPCPermission, UnknownValueHasNoName)
{
  EXPECT_STREQ("Unknown", PermissionToString(static_cast<OperationPermission>(0)));
  EXPECT_STREQ("Unknown", PermissionToString(static_cast<OperationPermission>(0x4000)));
}

//! \brief Every permission the shipped service description names is one that exists
TEST(TestJSONRPCPermission, EveryPermissionTheSchemaDeclaresExists)
{
  for (const auto& [name, method] : ShippedMethods())
  {
    const CVariant& declared = method["permission"];
    ASSERT_FALSE(declared.isNull()) << name;
    if (declared.isArray())
    {
      for (unsigned int index = 0; index < declared.size(); index++)
        EXPECT_TRUE(StringToPermission(declared[index].asString()).has_value())
            << name << " declares " << declared[index].asString();
    }
    else
    {
      EXPECT_TRUE(StringToPermission(declared.asString()).has_value())
          << name << " declares " << declared.asString();
    }
  }
}

TEST(TestJSONRPCPermission, EverySettingsWriteRequiresWriteSetting)
{
  for (const char* const name : {"Settings.SetSettingValue", "Settings.ResetSettingValue",
                                 "Settings.SetSkinSettingValue", "Settings.SetLevel"})
    EXPECT_EQ("WriteSetting", ShippedMethod(name)["permission"].asString()) << name;
}

TEST(TestJSONRPCPermission, AllCoversEveryPermission)
{
  int combined = 0;
  for (const auto permission : PERMISSIONS)
    combined |= permission;

  EXPECT_EQ(OPERATION_PERMISSION_ALL, combined);
}

//! \brief Notifications are pushed to a client that never asked for them, so they must not
//! require the read permission that gates ordinary queries
TEST(TestJSONRPCPermission, NotificationPermissionIsAllButReadData)
{
  EXPECT_EQ(OPERATION_PERMISSION_ALL & ~ReadData, OPERATION_PERMISSION_NOTIFICATION);
}
