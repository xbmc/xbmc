/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseOperations.h"

#include "DatabaseManager.h"
#include "ServiceBroker.h"
#include "utils/Variant.h"

using namespace JSONRPC;

JSONRPC_STATUS CDatabaseOperations::GetFullDatabaseNameByType(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string dbType = parameterObject["type"].asString();
  result = CServiceBroker::GetDatabaseManager().GetFullDatabaseNameByType(dbType);
  return OK;
}
