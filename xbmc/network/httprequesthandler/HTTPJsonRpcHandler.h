/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

class CHTTPJsonRpcHandler : public IHTTPRequestHandler
{
public:
  CHTTPJsonRpcHandler() = default;
  ~CHTTPJsonRpcHandler() override = default;

  // implementations of IHTTPRequestHandler
  IHTTPRequestHandler* Create(const HTTPRequest &request) const override { return new CHTTPJsonRpcHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request) const override;

  int HandleRequest() override;

  HttpResponseRanges GetResponseData() const override;

  int GetPriority() const override { return 5; }

protected:
  explicit CHTTPJsonRpcHandler(const HTTPRequest &request)
    : IHTTPRequestHandler(request)
  { }

  bool appendPostData(const char *data, size_t size) override;

private:
  std::string m_requestData;
  std::string m_responseData;
  CHttpResponseRange m_responseRange;

  class CHTTPTransportLayer : public JSONRPC::ITransportLayer
  {
  public:
    CHTTPTransportLayer() = default;
    ~CHTTPTransportLayer() override = default;

    // implementations of JSONRPC::ITransportLayer
    bool PrepareDownload(const char *path, CVariant &details, std::string &protocol) override;
    bool Download(const char *path, CVariant &result) override;
    int GetCapabilities() override;
  };
  CHTTPTransportLayer m_transportLayer;

  class CHTTPClient : public JSONRPC::IClient
  {
  public:
    explicit CHTTPClient(HTTPMethod method);
    ~CHTTPClient() override = default;

    int GetPermissionFlags() override { return m_permissionFlags; }
    int GetAnnouncementFlags() override;
    bool SetAnnouncementFlags(int flags) override;

  private:
    int m_permissionFlags;
  };
};
