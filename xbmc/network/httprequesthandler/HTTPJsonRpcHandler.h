#pragma once
/*
 *      Copyright (C) 2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "IHTTPRequestHandler.h"
#include "interfaces/json-rpc/IClient.h"

class CHTTPJsonRpcHandler : public IHTTPRequestHandler
{
public:
  CHTTPJsonRpcHandler() { };

  virtual bool CheckHTTPRequest(struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version);

#if (MHD_VERSION >= 0x00040001)
  virtual int HandleHTTPRequest(CWebServer *server, struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version,
                                const char *upload_data, size_t *upload_data_size, void **con_cls);
#else
  virtual int HandleHTTPRequest(CWebServer *server, struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version,
                                const char *upload_data, unsigned int *upload_data_size, void **con_cls);
#endif

  virtual void* GetHTTPResponseData() const { return (void *)m_response.c_str(); };
  virtual size_t GetHTTPResonseDataLength() const { return m_response.size(); }

  virtual int GetPriority() const { return 2; }

private:
  std::string m_response;

  class CHTTPClient : public JSONRPC::IClient
  {
  public:
    virtual int  GetPermissionFlags();
    virtual int  GetAnnouncementFlags();
    virtual bool SetAnnouncementFlags(int flags);
  };
};
