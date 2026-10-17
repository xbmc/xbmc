/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IHttpClient.h"
#include "filesystem/wasm/XhrFile.h"

namespace XFILE
{
class CXhrHttpClient : public IHttpClient
{
public:
  bool Get(const std::string& url, std::string& html) override { return m_xhr.Get(url, html); }
  bool Post(const std::string& url, const std::string& postData, std::string& html) override
  {
    return m_xhr.Post(url, postData, html);
  }
  bool IsInternet() override { return m_xhr.IsInternet(); }
  void Cancel() override { m_xhr.Cancel(); }
  void Reset() override { m_xhr.Reset(); }
  void SetUserAgent(const std::string& userAgent) override { m_xhr.SetUserAgent(userAgent); }
  void SetTimeout(int connectTimeoutSeconds) override { m_xhr.SetTimeout(connectTimeoutSeconds); }
  void SetReferer(const std::string& referer) override { m_xhr.SetReferer(referer); }
  std::string GetProperty(XFILE::FileProperty type, const std::string& name = "") const override
  {
    return m_xhr.GetProperty(type, name);
  }

private:
  CXhrFile m_xhr;
};
} // namespace XFILE
