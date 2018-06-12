/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "interfaces/json-rpc/ITransportLayer.h"
#include "interfaces/json-rpc/JSONRPC.h"

class CVariant;

class CAddOnTransport : public JSONRPC::ITransportLayer
{
public:
  bool PrepareDownload(const char *path, CVariant &details, std::string &protocol) override { return false; }
  bool Download(const char *path, CVariant& result) override { return false; }
  int GetCapabilities() override { return JSONRPC::Response; }

  class CAddOnClient : public JSONRPC::IClient
  {
  public:
    int  GetPermissionFlags() override { return JSONRPC::OPERATION_PERMISSION_ALL; }
    int  GetAnnouncementFlags() override { return 0; }
    bool SetAnnouncementFlags(int flags) override { return true; }
  };
};
