/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/IAnnouncer.h"
#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "interfaces/json-rpc/JSONRPCUtils.h"
#include "utils/Variant.h"

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
class CFlagsClient : public IClient
{
public:
  explicit CFlagsClient(int flags) : m_flags(flags) {}

  int GetPermissionFlags() override { return OPERATION_PERMISSION_ALL; }
  int GetAnnouncementFlags() override { return m_flags; }
  bool SetAnnouncementFlags(int flags) override
  {
    m_flags = flags;
    return true;
  }

private:
  int m_flags;
};

int Configure(CFlagsClient& client, const CVariant& notifications)
{
  CVariant params(CVariant::VariantTypeObject);
  params["notifications"] = notifications;
  CVariant result;
  EXPECT_EQ(
      OK, CJSONRPC::SetConfiguration("JSONRPC.SetConfiguration", nullptr, &client, params, result));
  return client.GetAnnouncementFlags();
}

const char* Name(int flag)
{
  return ANNOUNCEMENT::AnnouncementFlagToString(static_cast<ANNOUNCEMENT::AnnouncementFlag>(flag));
}
} // namespace

TEST(TestSetConfiguration, OmittedNamespacesKeepTheirCurrentState)
{
  CFlagsClient client(ANNOUNCEMENT::ANNOUNCE_ALL);
  CVariant notifications(CVariant::VariantTypeObject);
  notifications["Player"] = false;

  EXPECT_EQ(ANNOUNCEMENT::ANNOUNCE_ALL & ~ANNOUNCEMENT::Player, Configure(client, notifications));
}

TEST(TestSetConfiguration, EveryNamespaceCanBeEnabled)
{
  CFlagsClient client(0);
  CVariant notifications(CVariant::VariantTypeObject);
  for (int flag = 1; flag <= ANNOUNCEMENT::ANNOUNCE_ALL; flag *= 2)
    notifications[Name(flag)] = true;

  EXPECT_EQ(ANNOUNCEMENT::ANNOUNCE_ALL, Configure(client, notifications));
}

TEST(TestSetConfiguration, EveryNamespaceCanBeDisabled)
{
  CFlagsClient client(ANNOUNCEMENT::ANNOUNCE_ALL);
  CVariant notifications(CVariant::VariantTypeObject);
  for (int flag = 1; flag <= ANNOUNCEMENT::ANNOUNCE_ALL; flag *= 2)
    notifications[Name(flag)] = false;

  EXPECT_EQ(0, Configure(client, notifications));
}

TEST(TestSetConfiguration, ApplicationIsKeptByItsOwnBit)
{
  CVariant notifications(CVariant::VariantTypeObject);
  notifications["Player"] = true;

  CFlagsClient application(ANNOUNCEMENT::Application);
  EXPECT_EQ(ANNOUNCEMENT::Application | ANNOUNCEMENT::Player,
            Configure(application, notifications));

  CFlagsClient other(ANNOUNCEMENT::Other);
  EXPECT_EQ(ANNOUNCEMENT::Other | ANNOUNCEMENT::Player, Configure(other, notifications));
}
