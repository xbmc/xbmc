/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "JSONRPC.h"
#include "addons/IAddon.h"

namespace ADDON
{
class CAddonDatabase;
}

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
    static void FillDetails(ADDON::AddonPtr addon,
                            const CVariant& fields,
                            CVariant& result,
                            ADDON::CAddonDatabase& addondb,
                            bool append = false);
  };
}
