#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "CurlFile.h"
#include "utils/XBMCTinyXML.h"

namespace XFILE
{
  class CDAVFile : public CCurlFile
  {
  public:
    CDAVFile(void);
    virtual ~CDAVFile(void);

    virtual bool Execute(const CURL& url);

    virtual bool Delete(const CURL& url);
    virtual bool Rename(const CURL& url, const CURL& urlnew);

    virtual bool Exists(const CURL& url);
    virtual int Stat(const CURL& url, struct __stat64* buffer);

    virtual int GetLastResponseCode() { return m_lastResponseCode; }

  private:
    bool ParseResponse(const TiXmlElement *pElement, struct __stat64* statBuffer);

    int m_lastResponseCode;
    CXBMCTinyXML m_davResponse;
  };
}
