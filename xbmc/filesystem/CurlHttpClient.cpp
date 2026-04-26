/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "CurlHttpClient.h"

using namespace XFILE;

bool CCurlHttpClient::Get(const std::string& url, std::string& html)
{
  return m_curl.Get(url, html);
}

bool CCurlHttpClient::Post(const std::string& url, const std::string& postData, std::string& html)
{
  return m_curl.Post(url, postData, html);
}

bool CCurlHttpClient::IsInternet()
{
  return m_curl.IsInternet();
}

void CCurlHttpClient::Cancel()
{
  m_curl.Cancel();
}

void CCurlHttpClient::Reset()
{
  m_curl.Reset();
}

void CCurlHttpClient::SetUserAgent(const std::string& userAgent)
{
  m_curl.SetUserAgent(userAgent);
}

void CCurlHttpClient::SetTimeout(int connectTimeoutSeconds)
{
  m_curl.SetTimeout(connectTimeoutSeconds);
}

void CCurlHttpClient::SetReferer(const std::string& referer)
{
  m_curl.SetReferer(referer);
}

std::string CCurlHttpClient::GetProperty(XFILE::FileProperty type, const std::string& name) const
{
  return m_curl.GetProperty(type, name);
}
