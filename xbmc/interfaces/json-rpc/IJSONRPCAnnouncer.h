#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  the Free Software Foundation, 51 Franklin Street, Suite 500, Boston, MA 02110, USA.
 *
 */

#include "interfaces/IAnnouncer.h"
#include "utils/JSONVariantWriter.h"

namespace JSONRPC
{
  class IJSONRPCAnnouncer : public ANNOUNCEMENT::IAnnouncer
  {
  public:
    virtual ~IJSONRPCAnnouncer() { }

  protected:
    static std::string AnnouncementToJSONRPC(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *method, const CVariant &data, bool compactOutput)
    {
      CVariant root;
      root["jsonrpc"] = "2.0";

      std::string namespaceMethod = ANNOUNCEMENT::AnnouncementFlagToString(flag);
      namespaceMethod += ".";
      namespaceMethod += method;
      root["method"] = namespaceMethod;

      root["params"]["data"] = data;
      root["params"]["sender"] = sender;

      return CJSONVariantWriter::Write(root, compactOutput);
    }
  };
}
