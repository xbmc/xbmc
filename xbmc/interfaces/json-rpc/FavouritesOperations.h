/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "JSONRPC.h"

class CVariant;

namespace JSONRPC
{
  class CFavouritesOperations : public CJSONUtils
  {
  public:
    static JSONRPC_STATUS GetFavourites(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS AddFavourite(const CVariant& parameterObject, CVariant& result);
  };
}
