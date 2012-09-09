/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  <http://www.gnu.org/licenses/>.
 *
 */
// NfoFile.h: interface for the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
#define AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "utils/XBMCTinyXML.h"
#include "addons/Scraper.h"
#include "utils/CharsetConverter.h"
#include "utils/XMLUtils.h"

class CNfoFile
{
public:
  CNfoFile() : m_doc(NULL), m_headofdoc(NULL), m_type(ADDON::ADDON_UNKNOWN) {}
  virtual ~CNfoFile() { Close(); }

  enum NFOResult
  {
    NO_NFO       = 0,
    FULL_NFO     = 1,
    URL_NFO      = 2,
    COMBINED_NFO = 3,
    ERROR_NFO    = 4
  };

  NFOResult Create(const CStdString&, const ADDON::ScraperPtr&, int episode=-1,
                   const CStdString& strPath2="");
  template<class T>
    bool GetDetails(T& details,const char* document=NULL, bool prioritise=false)
  {
    CXBMCTinyXML doc;
    CStdString strDoc;
    if (document)
      strDoc = document;
    else
      strDoc = m_headofdoc;

    CStdString encoding;
    XMLUtils::GetEncoding(&doc, encoding);

    CStdString strUtf8(strDoc);
    if (encoding.IsEmpty())
      g_charsetConverter.unknownToUTF8(strUtf8);
    else
      g_charsetConverter.stringCharsetToUtf8(encoding, strDoc, strUtf8);

    doc.Clear();
    doc.Parse(strUtf8.c_str(),0,TIXML_ENCODING_UTF8);
    return details.Load(doc.RootElement(), true, prioritise);
  }

  void Close();
  void SetScraperInfo(const ADDON::ScraperPtr& info) { m_info = info; }
  const ADDON::ScraperPtr& GetScraperInfo() const { return m_info; }
  const CScraperUrl &ScraperUrl() const { return m_scurl; }

private:
  char* m_doc;
  char* m_headofdoc;
  ADDON::ScraperPtr m_info;
  ADDON::TYPE m_type;
  CScraperUrl m_scurl;

  int Load(const CStdString&);
  int Scrape(ADDON::ScraperPtr& scraper);
};

#endif // !defined(AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
