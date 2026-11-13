/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "JSONRPC.h"
#include "rendering/RenderSystemTypes.h"

class CVariant;

namespace JSONRPC
{
  class CGUIOperations
  {
  public:
    static JSONRPC_STATUS GetProperties(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS ActivateWindow(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS ShowNotification(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetFullscreen(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetStereoscopicMode(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetStereoscopicModes(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS ActivateScreenSaver(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS TakeScreenshot(const CVariant& parameterObject, CVariant& result);

  private:
    static JSONRPC_STATUS GetPropertyValue(const std::string &property, CVariant &result);
    static CVariant GetStereoModeObjectFromGuiMode(const RenderStereoMode mode);
  };
}
