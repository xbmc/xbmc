/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/IAnnouncer.h"
#include "utils/JSONVariantWriter.h"
#include "utils/Variant.h"

namespace JSONRPC
{
  class IJSONRPCAnnouncer : public ANNOUNCEMENT::IAnnouncer
  {
  public:
    ~IJSONRPCAnnouncer() override = default;

  protected:
    static std::string AnnouncementToJSONRPC(ANNOUNCEMENT::AnnouncementFlag flag,
                                             const std::string& sender,
                                             const std::string& method,
                                             const CVariant& data,
                                             bool compactOutput)
    {
      CVariant root;
      root["jsonrpc"] = "2.0";

      std::string namespaceMethod = ANNOUNCEMENT::AnnouncementFlagToString(flag);
      namespaceMethod += ".";
      namespaceMethod += method;
      root["method"] = namespaceMethod;

      root["params"]["data"] = data;
      root["params"]["sender"] = sender;

      std::string str;
      CJSONVariantWriter::Write(root, str, compactOutput);

      return str;
    }
  };
}
