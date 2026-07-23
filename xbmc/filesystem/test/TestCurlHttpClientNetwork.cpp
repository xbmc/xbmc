/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ServiceBroker.h"
#include "filesystem/CurlHttpClient.h"
#include "filesystem/HttpClientFactory.h"
#include "filesystem/IHttpClient.h"
#include "network/DNSNameCache.h"
#include "network/WebServer.h"
#include "network/httprequesthandler/HTTPRequestHandlerUtils.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"
#include "utils/HttpRangeUtils.h"
#include "utils/StringUtils.h"

#include <random>
#include <string>

#include <gtest/gtest.h>

#define WEBSERVER_HOST "localhost"

namespace
{

/*!
 * \brief Minimal in-process HTTP handler used as a test double for the
 *        network tests below:
 *
 *   GET  /get          → returns the body "hello from get"
 *   POST /post         → echoes back whatever was sent as the request body
 *   GET  /echo-headers → returns "User-Agent:<value>\nReferer:<value>"
 */
class CHTTPTestHandler : public IHTTPRequestHandler
{
public:
  CHTTPTestHandler() = default;
  explicit CHTTPTestHandler(const HTTPRequest& request) : IHTTPRequestHandler(request) {}

  IHTTPRequestHandler* Create(const HTTPRequest& request) const override
  {
    return new CHTTPTestHandler(request);
  }

  bool CanHandleRequest(const HTTPRequest& request) const override
  {
    return request.pathUrl == "/get" || request.pathUrl == "/post" ||
           request.pathUrl == "/echo-headers";
  }

  MHD_RESULT HandleRequest() override
  {
    if (m_request.pathUrl == "/get")
      m_responseBody = "hello from get";
    else if (m_request.pathUrl == "/post")
      m_responseBody = m_requestBody;
    else if (m_request.pathUrl == "/echo-headers")
    {
      const std::string userAgent = HTTPRequestHandlerUtils::GetRequestHeaderValue(
          m_request.connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_USER_AGENT);
      const std::string referer = HTTPRequestHandlerUtils::GetRequestHeaderValue(
          m_request.connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_REFERER);
      m_responseBody = "User-Agent:" + userAgent + "\nReferer:" + referer;
    }
    else
    {
      m_response.type = HTTPError;
      m_response.status = MHD_HTTP_NOT_FOUND;
      return MHD_YES;
    }

    m_responseRange.SetData(m_responseBody.c_str(), m_responseBody.size());
    m_response.type = HTTPMemoryDownloadNoFreeCopy;
    m_response.status = MHD_HTTP_OK;
    m_response.contentType = "text/plain";
    m_response.totalLength = m_responseBody.size();
    return MHD_YES;
  }

  HttpResponseRanges GetResponseData() const override { return {m_responseRange}; }

  bool appendPostData(const char* data, size_t size) override
  {
    m_requestBody.append(data, size);
    return true;
  }

private:
  std::string m_requestBody;
  std::string m_responseBody;
  mutable CHttpResponseRange m_responseRange;
};

} // namespace

class TestCurlHttpClientNetwork : public testing::Test
{
protected:
  TestCurlHttpClientNetwork()
  {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint16_t> dist(49152, 65535);
    m_port = dist(mt);
    m_baseUrl = StringUtils::Format("http://" WEBSERVER_HOST ":{}", m_port);
  }

  void SetUp() override
  {
    CServiceBroker::RegisterDNSNameCache(std::make_shared<CDNSNameCache>());
    m_webServer.Start(m_port, "", "");
    m_webServer.RegisterRequestHandler(&m_handler);
  }

  void TearDown() override
  {
    if (m_webServer.IsStarted())
      m_webServer.Stop();
    m_webServer.UnregisterRequestHandler(&m_handler);
    CServiceBroker::UnregisterDNSNameCache();
  }

  std::string Url(const std::string& path) const { return m_baseUrl + path; }

  CWebServer m_webServer;
  CHTTPTestHandler m_handler;
  uint16_t m_port{0};
  std::string m_baseUrl;
};

TEST_F(TestCurlHttpClientNetwork, ServerIsRunning)
{
  ASSERT_TRUE(m_webServer.IsStarted());
}

TEST_F(TestCurlHttpClientNetwork, GetReturnsBody)
{
  XFILE::CCurlHttpClient client;
  std::string response;
  ASSERT_TRUE(client.Get(Url("/get"), response));
  EXPECT_EQ(response, "hello from get");
}

TEST_F(TestCurlHttpClientNetwork, PostEchoesBody)
{
  XFILE::CCurlHttpClient client;
  std::string response;
  ASSERT_TRUE(client.Post(Url("/post"), "test payload", response));
  EXPECT_EQ(response, "test payload");
}

TEST_F(TestCurlHttpClientNetwork, GetViaFactoryInterface)
{
  auto client = XFILE::CreateHttpClient();
  std::string response;
  ASSERT_TRUE(client->Get(Url("/get"), response));
  EXPECT_EQ(response, "hello from get");
}

TEST_F(TestCurlHttpClientNetwork, SetUserAgentIsForwardedInRequest)
{
  XFILE::CCurlHttpClient client;
  client.SetUserAgent("KodiTest/1.0");

  std::string response;
  ASSERT_TRUE(client.Get(Url("/echo-headers"), response));
  EXPECT_NE(response.find("KodiTest/1.0"), std::string::npos);
}

TEST_F(TestCurlHttpClientNetwork, SetRefererIsForwardedInRequest)
{
  XFILE::CCurlHttpClient client;
  client.SetReferer("http://kodi.tv");

  std::string response;
  ASSERT_TRUE(client.Get(Url("/echo-headers"), response));
  EXPECT_NE(response.find("http://kodi.tv"), std::string::npos);
}
