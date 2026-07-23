/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// NfoFile.h: interface for the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#include "InfoScanner.h"
#include "URL.h"
#include "addons/Scraper.h"
#include "utils/XBMCTinyXML.h"

#include <string>
#include <utility>
#include <vector>

namespace ADDON
{
enum class AddonType;
}

class CNfoFile
{
public:
  virtual ~CNfoFile() { Close(); }

  CInfoScanner::InfoType Create(const std::string&, const ADDON::ScraperPtr&, int index = 1);

  template<class T>
  bool GetDetails(T& details, const char* document = nullptr, bool prioritise = false) const
  {
    if (document)
    {
      CXBMCTinyXML doc;
      doc.Parse(document, TIXML_ENCODING_UNKNOWN);
      return details.Load(doc.RootElement(), true, prioritise);
    }

    const TiXmlElement* root = GetRootElement();
    if (!root)
      return false;

    return details.Load(root, true, prioritise);
  }

  void Close();
  void SetScraperInfo(ADDON::ScraperPtr info) { m_info = std::move(info); }
  ADDON::ScraperPtr GetScraperInfo() { return m_info; }
  const CScraperUrl &ScraperUrl() const { return m_scurl; }

private:
  CInfoScanner::InfoType TryParsing(ADDON::AddonType addonType) const;
  CInfoScanner::InfoType TryParsing(const CURL& nfoPath,
                                    ADDON::ContentType contentType,
                                    int index = 1);
  CInfoScanner::InfoType SearchNfoForScraperUrls(CInfoScanner::InfoType parseResult,
                                                 const ADDON::ScraperPtr& info);
  bool SeekToMovieIndex(int index);

  // Returns the root element of m_doc (from m_headPos)
  const TiXmlElement* GetRootElement() const;

  std::string m_doc;
  size_t m_headPos = 0;
  ADDON::ScraperPtr m_info;
  CScraperUrl m_scurl;

  mutable CXBMCTinyXML m_xmlDoc;
  mutable bool m_xmlParsed = false;

  int Load(const CURL&);
};
