#pragma once
/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "JSONRPC.h"
#include "addons/IAddon.h"

class CAddonDatabase;
class CVariant;

namespace JSONRPC
{
  class CAddonsOperations : public CJSONUtils
  {
  public:
    static JSONRPC_STATUS GetAddons(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetAddonDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    
    static JSONRPC_STATUS SetAddonEnabled(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS ExecuteAddon(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    
  private:
    static void FillDetails(ADDON::AddonPtr addon, const CVariant& fields, CVariant &result, CAddonDatabase &addondb, bool append = false);
  };
}
