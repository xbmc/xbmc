/*
 *      Copyright (C) 2015 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <string>

#include "XBDateTime.h"
#include "filesystem/File.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

class CHTTPFileHandler : public IHTTPRequestHandler
{
public:
  ~CHTTPFileHandler() override = default;

  int HandleRequest() override;

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
