/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "JSONRPC.h"

#include <string>

class CVariant;
class ITransportLayer;
class IClient;

namespace JSONRPC
{
class CDatabaseOperations
{
public:
  static JSONRPC_STATUS GetDatabaseNameByType(const std::string& method,
                                              ITransportLayer* transport,
                                              IClient* client,
                                              const CVariant& parameterObject,
                                              CVariant& result);
};
} // namespace JSONRPC
