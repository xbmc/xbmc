#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

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
    virtual ~ITransportLayer() { };
    virtual bool PrepareDownload(const char *path, CVariant &details, std::string &protocol) = 0;
    virtual bool Download(const char *path, CVariant &result) = 0;
    virtual int GetCapabilities() = 0;
  };
}
