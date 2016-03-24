#pragma once
/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include <string>

#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

class CHTTPJsonRpcHandler : public IHTTPRequestHandler
{
public:
  CHTTPJsonRpcHandler() { }
  virtual ~CHTTPJsonRpcHandler() { }
  
  // implementations of IHTTPRequestHandler
  virtual IHTTPRequestHandler* Create(const HTTPRequest &request) { return new CHTTPJsonRpcHandler(request); }
  virtual bool CanHandleRequest(const HTTPRequest &request);

  virtual int HandleRequest();

  virtual HttpResponseRanges GetResponseData() const;

  virtual int GetPriority() const { return 5; }

protected:
  explicit CHTTPJsonRpcHandler(const HTTPRequest &request)
    : IHTTPRequestHandler(request)
  { }

#if (MHD_VERSION >= 0x00040001)
  virtual bool appendPostData(const char *data, size_t size);
#else
  virtual bool appendPostData(const char *data, unsigned int size);
#endif

private:
  std::string m_requestData;
  std::string m_responseData;
  CHttpResponseRange m_responseRange;

  class CHTTPTransportLayer : public JSONRPC::ITransportLayer
  {
  public:
    CHTTPTransportLayer() = default;
    ~CHTTPTransportLayer() = default;

    // implementations of JSONRPC::ITransportLayer
    bool PrepareDownload(const char *path, CVariant &details, std::string &protocol) override;
    bool Download(const char *path, CVariant &result) override;
    int GetCapabilities() override;
  };
  CHTTPTransportLayer m_transportLayer;

  class CHTTPClient : public JSONRPC::IClient
  {
  public:
    virtual int  GetPermissionFlags();
    virtual int  GetAnnouncementFlags();
    virtual bool SetAnnouncementFlags(int flags);
  };
};
