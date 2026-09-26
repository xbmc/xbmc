/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "network/DNSNameCache.h"

#include <array>
#include <cstring>
#include <string>

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

TEST(TestDNSNameCache, LocalhostIsResolvedAndCached)
{
  CDNSNameCache cache;
  std::string address;
  ASSERT_TRUE(cache.Lookup("localhost", address));
  EXPECT_TRUE(address == "::1" || address == "127.0.0.1") << address;

  std::string cached;
  ASSERT_TRUE(cache.GetCached("localhost", cached));
  EXPECT_EQ(address, cached);
}

namespace
{
struct AddressSelectionTest
{
  std::array<const char*, 2> addresses;
  const char* expected;
};
} // namespace

TEST(TestDNSNameCache, SelectAddress)
{
  const std::array tests{
      AddressSelectionTest{{"::1", "192.0.2.1"}, "192.0.2.1"},
      AddressSelectionTest{{"127.42.0.7", "192.0.2.1"}, "192.0.2.1"},
      AddressSelectionTest{{"::", "192.0.2.1"}, "192.0.2.1"},
      AddressSelectionTest{{"0.0.0.0", "2001:db8::1"}, "2001:db8::1"},
      AddressSelectionTest{{"::ffff:127.0.0.1", "192.0.2.1"}, "192.0.2.1"},
      AddressSelectionTest{{"::ffff:0.0.0.0", "192.0.2.1"}, "192.0.2.1"},
      AddressSelectionTest{{"::1", nullptr}, "::1"},
      AddressSelectionTest{{"127.42.0.7", nullptr}, "127.42.0.7"},
      AddressSelectionTest{{"::1", "127.0.0.1"}, "::1"},
      AddressSelectionTest{{"0.0.0.0", "::"}, "0.0.0.0"},
      AddressSelectionTest{{"192.0.2.1", "2001:db8::1"}, "192.0.2.1"},
      AddressSelectionTest{{"2001:db8::1", "192.0.2.1"}, "2001:db8::1"},
      AddressSelectionTest{{"::ffff:192.0.2.1", "2001:db8::1"}, "::ffff:192.0.2.1"},
  };

  for (const auto& test : tests)
  {
    SCOPED_TRACE(test.addresses[0]);
    std::array<sockaddr_storage, 2> addresses{};
    std::array<addrinfo, 2> results{};
    for (size_t i = 0; i < test.addresses.size() && test.addresses[i]; ++i)
    {
      auto& result = results[i];
      result.ai_family = std::strchr(test.addresses[i], ':') ? AF_INET6 : AF_INET;
      result.ai_socktype = SOCK_STREAM;
      result.ai_addr = reinterpret_cast<sockaddr*>(&addresses[i]);
      if (result.ai_family == AF_INET)
      {
        auto* address = reinterpret_cast<sockaddr_in*>(&addresses[i]);
        address->sin_family = AF_INET;
        result.ai_addrlen = sizeof(*address);
        ASSERT_EQ(1, inet_pton(AF_INET, test.addresses[i], &address->sin_addr));
      }
      else
      {
        auto* address = reinterpret_cast<sockaddr_in6*>(&addresses[i]);
        address->sin6_family = AF_INET6;
        result.ai_addrlen = sizeof(*address);
        ASSERT_EQ(1, inet_pton(AF_INET6, test.addresses[i], &address->sin6_addr));
      }
      if (i > 0)
        results[i - 1].ai_next = &result;
    }

    EXPECT_EQ(test.expected, KODI::NETWORK::SelectDNSAddress(results.data()));
  }
}
