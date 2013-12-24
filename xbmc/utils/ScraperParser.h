#ifndef SCRAPER_PARSER_H
#define SCRAPER_PARSER_H

/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include <vector>
#include "StdString.h"
#include "addons/IAddon.h"

#define MAX_SCRAPER_BUFFERS 20

namespace ADDON
{
  class CScraper;
}

class TiXmlElement;
class CXBMCTinyXML;

class CScraperSettings;

class CScraperParser
{
public:
  CScraperParser();
  CScraperParser(const CScraperParser& parser);
  ~CScraperParser();
  CScraperParser& operator= (const CScraperParser& parser);
  bool Load(const CStdString& strXMLFile);
  bool IsNoop() { return m_isNoop; };

  void Clear();
  const CStdString GetFilename() { return m_strFile; }
  CStdString GetSearchStringEncoding() const
    { return m_SearchStringEncoding; }
  const CStdString Parse(const CStdString& strTag,
                         ADDON::CScraper* scraper);

  void AddDocument(const CXBMCTinyXML* doc);

  CStdString m_param[MAX_SCRAPER_BUFFERS];

private:
  bool LoadFromXML();
  void ReplaceBuffers(CStdString& strDest);
  void ParseExpression(const CStdString& input, CStdString& dest, TiXmlElement* element, bool bAppend);

  /*! \brief Parse an 'XSLT' declaration from the scraper
   This allow us to transform an inbound XML document using XSLT
   to a different type of XML document, ready to be output direct
   to the album loaders or similar
   \param input the input document
   \param dest the output destation for the conversion
   \param element the current XML element
   \param bAppend append or clear the buffer
   */
  void ParseXSLT(const CStdString& input, CStdString& dest, TiXmlElement* element, bool bAppend);
  void ParseNext(TiXmlElement* element);
  void Clean(CStdString& strDirty);
  void ConvertJSON(CStdString &string);
  void ClearBuffers();
  void GetBufferParams(bool* result, const char* attribute, bool defvalue);
  void InsertToken(CStdString& strOutput, int buf, const char* token);

  CXBMCTinyXML* m_document;
  TiXmlElement* m_pRootElement;

  const char* m_SearchStringEncoding;
  bool m_isNoop;

  CStdString m_strFile;
  ADDON::CScraper* m_scraper;
};

#endif


