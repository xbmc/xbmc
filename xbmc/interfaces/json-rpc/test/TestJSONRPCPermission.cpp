/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/json-rpc/JSONRPCUtils.h"

#include <array>
#include <string_view>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
constexpr std::array<OperationPermission, 13> PERMISSIONS{
    ReadData,  ControlPlayback, ControlNotify, ControlPower, UpdateData,   RemoveData, Navigate,
    WriteFile, ControlSystem,   ControlGUI,    ManageAddon,  ExecuteAddon, ControlPVR};
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

//! \brief StringToPermission has no failure signal - anything it does not recognise reads as
//! ReadData, so a misspelled permission in a schema silently becomes the least privileged one
TEST(TestJSONRPCPermission, UnrecognisedNameFallsBackToReadData)
{
  EXPECT_EQ(ReadData, StringToPermission("NoSuchPermission"));
  EXPECT_EQ(ReadData, StringToPermission(""));
  EXPECT_EQ(ReadData, StringToPermission("Unknown"));
  EXPECT_EQ(ReadData, StringToPermission("controlpvr")) << "matching is case sensitive";
}

TEST(TestJSONRPCPermission, UnknownValueHasNoName)
{
  EXPECT_STREQ("Unknown", PermissionToString(static_cast<OperationPermission>(0)));
  EXPECT_STREQ("Unknown", PermissionToString(static_cast<OperationPermission>(0x2000)));
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
