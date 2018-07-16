/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace JSONRPC
{
  class IClient
  {
  public:
    virtual ~IClient() = default;
    virtual int GetPermissionFlags() = 0;
    virtual int GetAnnouncementFlags() = 0;
    virtual bool SetAnnouncementFlags(int flags) = 0;
  };
}
