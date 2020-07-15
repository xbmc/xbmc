/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

  MHD_RESULT HandleRequest() override;

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
