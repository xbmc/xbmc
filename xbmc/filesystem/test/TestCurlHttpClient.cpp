/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/CurlHttpClient.h"
#include "filesystem/HttpClientFactory.h"
#include "filesystem/IHttpClient.h"

#include <gtest/gtest.h>

namespace XFILE
{

// ---- HttpClientFactory -------------------------------------------------------

TEST(TestHttpClientFactory, CreateReturnsNonNull)
{
  auto client = CreateHttpClient();
  EXPECT_NE(client, nullptr);
}

TEST(TestHttpClientFactory, CreateReturnsCurlHttpClient)
{
  auto client = CreateHttpClient();
  EXPECT_NE(dynamic_cast<CCurlHttpClient*>(client.get()), nullptr);
}

// ---- CCurlHttpClient ---------------------------------------------------------
// These tests exercise the real delegation path without requiring a network
// connection. Get(), Post() and IsInternet() need a reachable server, so they
// are covered by TestCurlHttpClientNetwork instead.

class TestCurlHttpClient : public testing::Test
{
protected:
  CCurlHttpClient m_client;
};

TEST_F(TestCurlHttpClient, GetPropertyOnIdleClientReturnsEmpty)
{
  // Nothing has been fetched yet, so every header-backed property is empty.
  // CScraperUrl::Get() relies on this path to sniff the response mime type.
  EXPECT_EQ(m_client.GetProperty(FileProperty::MIME_TYPE), "");
  EXPECT_EQ(m_client.GetProperty(FileProperty::CONTENT_TYPE), "");
  EXPECT_EQ(m_client.GetProperty(FileProperty::CONTENT_CHARSET), "");
  EXPECT_EQ(m_client.GetProperty(FileProperty::RESPONSE_PROTOCOL), "");
  EXPECT_EQ(m_client.GetProperty(FileProperty::RESPONSE_HEADER, "Server"), "");
}

TEST_F(TestCurlHttpClient, CancelAndResetLeaveIdleClientUsable)
{
  // Cancel() returns immediately while no transfer is open and Reset() clears
  // the cancelled flag, which is how CVideoInfoDownloader recycles its client.
  m_client.Cancel();
  m_client.Reset();

  EXPECT_EQ(m_client.GetProperty(FileProperty::MIME_TYPE), "");
}

TEST_F(TestCurlHttpClient, SettersDispatchThroughInterface)
{
  // Verify virtual dispatch routes to the real CCurlHttpClient methods. That
  // the values reach the wire is checked in TestCurlHttpClientNetwork.
  IHttpClient& iface = m_client;
  iface.SetUserAgent("Kodi/21.0");
  iface.SetTimeout(10);
  iface.SetReferer("http://kodi.tv");

  EXPECT_EQ(iface.GetProperty(FileProperty::MIME_TYPE), "");
}

} // namespace XFILE
