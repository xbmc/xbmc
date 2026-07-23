/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
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
// connection. Cancel(), Reset() and GetProperty() dereference m_state which
// is only allocated on Open(), so they are not safe to call here.

class TestCurlHttpClient : public testing::Test
{
protected:
  CCurlHttpClient m_client;
};

TEST_F(TestCurlHttpClient, SetUserAgent)
{
  m_client.SetUserAgent("Kodi/21.0");
}

TEST_F(TestCurlHttpClient, SetTimeout)
{
  m_client.SetTimeout(30);
}

TEST_F(TestCurlHttpClient, SetReferer)
{
  m_client.SetReferer("http://kodi.tv");
}

TEST_F(TestCurlHttpClient, StateSetstersDispatchThroughInterface)
{
  // Verify virtual dispatch routes to the real CCurlHttpClient methods.
  IHttpClient& iface = m_client;
  iface.SetUserAgent("Kodi/21.0");
  iface.SetTimeout(10);
  iface.SetReferer("http://kodi.tv");
}

} // namespace XFILE
