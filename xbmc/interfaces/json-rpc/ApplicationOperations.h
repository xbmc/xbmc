/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItemHandler.h"
#include "JSONRPC.h"

#include <optional>
#include <string>

class CVariant;

namespace JSONRPC
{
  class CApplicationOperations : CFileItemHandler
  {
  public:
    static JSONRPC_STATUS GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS SetVolume(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetMute(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetLogLevel(const std::string& method,
                                      ITransportLayer* transport,
                                      IClient* client,
                                      const CVariant& parameterObject,
                                      CVariant& result);

    static JSONRPC_STATUS Quit(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

  protected:
    /*!
     \brief The Application.LogLevel name of a log level; empty for a value outside the scale
     */
    static std::string LogLevelName(int level);

    /*!
     \brief The log level an Application.LogLevel name stands for; nothing for an unknown name
     */
    static std::optional<int> LogLevelFromName(const std::string& name);

  private:
    static JSONRPC_STATUS GetPropertyValue(const std::string &property, CVariant &result);
    static CVariant LogLevelValue();
  };
}
