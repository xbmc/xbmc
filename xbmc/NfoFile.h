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
// NfoFile.h: interface for the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
#define AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_

#pragma once
#include <string>

#include "addons/Scraper.h"
#include "utils/CharsetConverter.h"

class CNfoFile
{
public:
  CNfoFile() : m_headPos(0), m_type(ADDON::ADDON_UNKNOWN) {}
  virtual ~CNfoFile() { Close(); }

  enum NFOResult
  {
    NO_NFO       = 0,
    FULL_NFO     = 1,
    URL_NFO      = 2,
    COMBINED_NFO = 3,
    ERROR_NFO    = 4
  };

  NFOResult Create(const std::string&, const ADDON::ScraperPtr&, int episode=-1);
  template<class T>
    bool GetDetails(T& details,const char* document=NULL, bool prioritise=false)
  {

    CXBMCTinyXML doc;
    if (document)
      doc.Parse(document, TIXML_ENCODING_UNKNOWN);
    else if (m_headPos < m_doc.size())
      doc.Parse(m_doc.substr(m_headPos), TIXML_ENCODING_UNKNOWN);
    else
      return false;

    return details.Load(doc.RootElement(), true, prioritise);
  }

  void Close();
  void SetScraperInfo(const ADDON::ScraperPtr& info) { m_info = info; }
  const ADDON::ScraperPtr& GetScraperInfo() const { return m_info; }
  const CScraperUrl &ScraperUrl() const { return m_scurl; }

private:
  std::string m_doc;
  size_t m_headPos;
  ADDON::ScraperPtr m_info;
  ADDON::TYPE m_type;
  CScraperUrl m_scurl;

  int Load(const std::string&);
  int Scrape(ADDON::ScraperPtr& scraper);
};

#endif // !defined(AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
