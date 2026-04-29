/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "IFileTypes.h"

#include <string>

namespace XFILE
{
class IHttpClient
{
public:
  virtual ~IHttpClient() = default;

  virtual bool Get(const std::string& url, std::string& html) = 0;
  virtual bool Post(const std::string& url, const std::string& postData, std::string& html) = 0;
  virtual bool IsInternet() = 0;

  virtual void Cancel() = 0;
  virtual void Reset() = 0;

  virtual void SetUserAgent(const std::string& userAgent) = 0;
  virtual void SetTimeout(int connectTimeoutSeconds) = 0;
  virtual void SetReferer(const std::string& referer) = 0;

  virtual std::string GetProperty(XFILE::FileProperty type, const std::string& name = "") const = 0;
};
} // namespace XFILE
