/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
