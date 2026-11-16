/*
 *  Copyright (C) 2005-2018 Team Kodi
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
  class CSystemOperations
  {
  public:
    static JSONRPC_STATUS GetProperties(ITransportLayer* transport,
                                        IClient* client,
                                        const CVariant& parameterObject,
                                        CVariant& result);

    static JSONRPC_STATUS EjectOpticalDrive(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS Shutdown(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Suspend(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Hibernate(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Reboot(const CVariant& parameterObject, CVariant& result);

  private:
    static JSONRPC_STATUS GetPropertyValue(int permissions, const std::string &property, CVariant &result);
  };
}
