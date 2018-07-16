/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CVariant;

namespace JSONRPC
{
  enum TransportLayerCapability
  {
    Response = 0x1,
    Announcing = 0x2,
    FileDownloadRedirect = 0x4,
    FileDownloadDirect = 0x8
  };

  #define TRANSPORT_LAYER_CAPABILITY_ALL (Response | Announcing | FileDownloadRedirect | FileDownloadDirect)

  class ITransportLayer
  {
  public:
    virtual ~ITransportLayer() = default;
    virtual bool PrepareDownload(const char *path, CVariant &details, std::string &protocol) = 0;
    virtual bool Download(const char *path, CVariant &result) = 0;
    virtual int GetCapabilities() = 0;
  };
}
