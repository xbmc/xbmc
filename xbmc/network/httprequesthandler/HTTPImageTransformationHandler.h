#pragma once
/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
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

#include <stdint.h>
#include <string>

#include "XBDateTime.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

class CHTTPImageTransformationHandler : public IHTTPRequestHandler
{
public:
  CHTTPImageTransformationHandler();
  virtual ~CHTTPImageTransformationHandler();

  virtual IHTTPRequestHandler* Create(const HTTPRequest &request) { return new CHTTPImageTransformationHandler(request); }
  virtual bool CanHandleRequest(const HTTPRequest &request);

  virtual int HandleRequest();

  virtual bool CanHandleRanges() const { return true; }
  virtual bool CanBeCached() const { return true; }
  virtual bool GetLastModifiedDate(CDateTime &lastModified) const;

  virtual HttpResponseRanges GetResponseData() const { return m_responseData; }

  // priority must be higher than the one of CHTTPImageHandler
  virtual int GetPriority() const { return 6; }

protected:
  explicit CHTTPImageTransformationHandler(const HTTPRequest &request);

private:
  std::string m_url;
  CDateTime m_lastModified;

  uint8_t* m_buffer;
  HttpResponseRanges m_responseData;
};
