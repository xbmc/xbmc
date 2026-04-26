/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "CurlFile.h"
#include "IHttpClient.h"

namespace XFILE
{
class CCurlHttpClient : public IHttpClient
{
public:
  bool Get(const std::string& url, std::string& html) override;
  bool Post(const std::string& url, const std::string& postData, std::string& html) override;
  bool IsInternet() override;

  void Cancel() override;
  void Reset() override;

  void SetUserAgent(const std::string& userAgent) override;
  void SetTimeout(int connectTimeoutSeconds) override;
  void SetReferer(const std::string& referer) override;

  std::string GetProperty(XFILE::FileProperty type, const std::string& name = "") const override;

private:
  CCurlFile m_curl;
};
} // namespace XFILE
