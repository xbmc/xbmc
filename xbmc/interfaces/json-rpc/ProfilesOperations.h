/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItemHandler.h"
#include "JSONRPC.h"

class CVariant;

namespace JSONRPC
{
  class CProfilesOperations : CFileItemHandler
  {
  public:
    static JSONRPC_STATUS GetProfiles(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetCurrentProfile(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS LoadProfile(const CVariant& parameterObject, CVariant& result);
  };
}
