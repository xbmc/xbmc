/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/json-rpc/JSONRPCUtils.h"

#include <array>
#include <set>
#include <string_view>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
//! Every JSONRPC_STATUS that reaches a client as an error. OK and ACK produce a result.
constexpr std::array<JSONRPC_STATUS, 9> ERROR_STATUSES{
    ParseError,      InvalidRequest, MethodNotFound, InvalidParams, InternalError,
    FailedToExecute, BadPermission,  NotFound,       Unavailable};
} // namespace

//! \brief A status added to JSONRPC_STATUS must also be described, or clients cannot discover it
TEST(TestJSONRPCStatus, EveryErrorStatusIsDescribed)
{
  for (const auto status : ERROR_STATUSES)
    EXPECT_NE(nullptr, StatusToDescription(status))
        << "undescribed status " << static_cast<int>(status);

  EXPECT_EQ(ERROR_STATUSES.size(), JSONRPC_STATUS_DESCRIPTIONS.size());
}

TEST(TestJSONRPCStatus, SuccessStatusesAreNotDescribed)
{
  EXPECT_EQ(nullptr, StatusToDescription(OK));
  EXPECT_EQ(nullptr, StatusToDescription(ACK));
}

TEST(TestJSONRPCStatus, CodesAndNamesAreUnique)
{
  std::set<int> codes;
  std::set<std::string_view> names;

  for (const auto& description : JSONRPC_STATUS_DESCRIPTIONS)
  {
    EXPECT_TRUE(codes.insert(static_cast<int>(description.status)).second)
        << "duplicate code " << static_cast<int>(description.status);
    EXPECT_TRUE(names.insert(description.name).second) << "duplicate name " << description.name;
  }
}

//! \brief CJSONRPC::BuildResponse populates "error.data" for InvalidParams and nothing else
TEST(TestJSONRPCStatus, OnlyInvalidParamsCarriesData)
{
  for (const auto& description : JSONRPC_STATUS_DESCRIPTIONS)
    EXPECT_EQ(description.status == InvalidParams, description.hasData) << description.name;
}

TEST(TestJSONRPCStatus, EveryDescriptionIsPopulated)
{
  for (const auto& description : JSONRPC_STATUS_DESCRIPTIONS)
  {
    EXPECT_FALSE(std::string_view(description.name).empty());
    EXPECT_FALSE(std::string_view(description.message).empty());
    EXPECT_FALSE(std::string_view(description.description).empty());
  }
}
