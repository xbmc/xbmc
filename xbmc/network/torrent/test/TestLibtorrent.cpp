/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "network/torrent/Libtorrent.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace NETWORK;

constexpr const char* TEST_MAGNET_LINK =
    "magnet:?xt=urn:btih:dd8255ecdc7ca55fb0bbf81323d87062db1f6d1c&dn=Big%20Buck%20Bunny";

class TestLibtorrent : public testing::Test
{
protected:
  void SetUp() override { libtorrent.Start(); }

  void TearDown() override
  {
    libtorrent.Stop(false);
    libtorrent.Stop(true);
  }

  CLibtorrent libtorrent;
};

TEST_F(TestLibtorrent, IsOnline)
{
  ASSERT_TRUE(libtorrent.IsOnline());
}

TEST_F(TestLibtorrent, DISABLED_AddTorrent)
{
  //! @todo: We might need to wait until the DHT is fully bootstrapped before
  //! we can add a torrent
  lt::torrent_handle torrentHandle = libtorrent.AddTorrent(TEST_MAGNET_LINK);
  ASSERT_TRUE(torrentHandle.is_valid());
}
