/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "JSONRPC.h"

#include <memory>

namespace ADDON
{
class CAddonDatabase;
class IAddon;
}

class CVariant;

namespace JSONRPC
{
  class CAddonsOperations : public CJSONUtils
  {
  public:
    static JSONRPC_STATUS GetAddons(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetAddonDetails(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS SetAddonEnabled(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS ExecuteAddon(const CVariant& parameterObject, CVariant& result);

  private:
    static void FillDetails(const std::shared_ptr<ADDON::IAddon>& addon,
                            const CVariant& fields,
                            CVariant& result,
                            bool append);
  };
}
