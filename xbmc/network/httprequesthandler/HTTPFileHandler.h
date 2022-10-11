/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

#include <string>

class CHTTPFileHandler : public IHTTPRequestHandler
{
public:
  ~CHTTPFileHandler() override = default;

  MHD_RESULT HandleRequest() override;

  bool CanHandleRanges() const override { return m_canHandleRanges; }
  bool CanBeCached() const override { return m_canBeCached; }
  bool GetLastModifiedDate(CDateTime &lastModified) const override;

  std::string GetRedirectUrl() const override { return m_url; }
  std::string GetResponseFile() const override { return m_url; }

protected:
  CHTTPFileHandler();
  explicit CHTTPFileHandler(const HTTPRequest &request);

  void SetFile(const std::string& file, int responseStatus);

  void SetCanHandleRanges(bool canHandleRanges) { m_canHandleRanges = canHandleRanges; }
  void SetCanBeCached(bool canBeCached) { m_canBeCached = canBeCached; }
  void SetLastModifiedDate(const struct __stat64 *buffer);

private:
  std::string m_url;

  bool m_canHandleRanges = true;
  bool m_canBeCached = true;

  CDateTime m_lastModified;

};
