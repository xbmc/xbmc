/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IDirectory.h"

namespace XFILE
{
  class CWin32SMBFile; // forward declaration

  class CWin32SMBDirectory : public IDirectory
  {
    friend class CWin32SMBFile;
  public:
    CWin32SMBDirectory(void);
    virtual ~CWin32SMBDirectory(void);
    virtual bool GetDirectory(const CURL& url, CFileItemList& items);
    virtual bool Create(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual bool Remove(const CURL& url);
  protected:
    bool RealCreate(const CURL& url, bool tryToConnect);
    bool RealExists(const CURL& url, bool tryToConnect);
    static bool GetNetworkResources(const CURL& basePath, CFileItemList& items);
    bool ConnectAndAuthenticate(CURL& url, bool allowPromptForCredential = false);
  };
}
