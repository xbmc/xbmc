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

#include <stdint.h>
#include <string>

#include "XBDateTime.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

class CHTTPImageTransformationHandler : public IHTTPRequestHandler
{
public:
  CHTTPImageTransformationHandler();
  ~CHTTPImageTransformationHandler() override;

  IHTTPRequestHandler* Create(const HTTPRequest &request) const override { return new CHTTPImageTransformationHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request)const  override;

  int HandleRequest() override;

  bool CanHandleRanges() const override { return true; }
  bool CanBeCached() const override { return true; }
  bool GetLastModifiedDate(CDateTime &lastModified) const override;

  HttpResponseRanges GetResponseData() const override { return m_responseData; }

  // priority must be higher than the one of CHTTPImageHandler
  int GetPriority() const override { return 6; }

protected:
  explicit CHTTPImageTransformationHandler(const HTTPRequest &request);

private:
  std::string m_url;
  CDateTime m_lastModified;

  uint8_t* m_buffer;
  HttpResponseRanges m_responseData;
};
