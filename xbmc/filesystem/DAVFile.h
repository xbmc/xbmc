/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "CurlFile.h"

namespace XFILE
{
  class CDAVFile : public CCurlFile
  {
  public:
    CDAVFile(void);
    ~CDAVFile(void) override;

    virtual bool Execute(const CURL& url);

    bool Delete(const CURL& url) override;
    bool Rename(const CURL& url, const CURL& urlnew) override;

    virtual int GetLastResponseCode() { return m_lastResponseCode; }

  private:
    int m_lastResponseCode = 0;
  };
}
