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
  class CInputOperations
  {
  public:
    static JSONRPC_STATUS SendText(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS ExecuteAction(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS ButtonEvent(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS Left(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Right(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Down(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Up(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS Select(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Back(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS ContextMenu(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Info(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Home(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS ShowCodec(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS ShowOSD(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS ShowPlayerProcessInfo(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS SendAction(int actionID, bool wakeScreensaver = true, bool waitResult = false);

  private:
    static JSONRPC_STATUS activateWindow(int windowID);
    static bool        handleScreenSaver();
  };
}
