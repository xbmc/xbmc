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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "system.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "interfaces/json-rpc/JSONRPC.h"

#ifdef HAS_JSONRPC
class CAddOnTransport : public JSONRPC::ITransportLayer
{
public:
  virtual bool PrepareDownload(const char *path, CVariant &details, std::string &protocol) { return false; }
  virtual bool Download(const char *path, CVariant& result) { return false; }
  virtual int GetCapabilities() { return JSONRPC::Response; }

  class CAddOnClient : public JSONRPC::IClient
  {
  public:
    virtual int  GetPermissionFlags() { return JSONRPC::OPERATION_PERMISSION_ALL; }
    virtual int  GetAnnouncementFlags() { return 0; }
    virtual bool SetAnnouncementFlags(int flags) { return true; }
  };
};
#endif
