/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#define MAX_SCRAPER_BUFFERS 20

namespace ADDON
{
  class CScraper;
}

namespace tinyxml2
{
class XMLElement;
class XMLDocument;
} // namespace tinyxml2
class CXBMCTinyXML2;

class CScraperSettings;

class CScraperParser
{
public:
  CScraperParser();
  CScraperParser(const CScraperParser& parser);
  ~CScraperParser();
  CScraperParser& operator= (const CScraperParser& parser);
  bool Load(const std::string& strXMLFile);
  bool IsNoop() const { return m_isNoop; }

  void Clear();
  const std::string& GetFilename() const { return m_strFile; }
  std::string GetSearchStringEncoding() const
    { return m_SearchStringEncoding; }
  const std::string Parse(const std::string& strTag,
                         ADDON::CScraper* scraper);

  void AddDocument(const CXBMCTinyXML2* doc);

  std::string m_param[MAX_SCRAPER_BUFFERS];

private:
  bool LoadFromXML();
  void ReplaceBuffers(std::string& strDest);
  void ParseExpression(const std::string& input,
                       std::string& dest,
                       tinyxml2::XMLElement* element,
                       bool bAppend);

  /*! \brief Parse an 'XSLT' declaration from the scraper
   This allow us to transform an inbound XML document using XSLT
   to a different type of XML document, ready to be output direct
   to the album loaders or similar
   \param input the input document
   \param dest the output destination for the conversion
   \param element the current XML element
   \param bAppend append or clear the buffer
   */
  void ParseXSLT(const std::string& input,
                 std::string& dest,
                 tinyxml2::XMLElement* element,
                 bool bAppend);
  void ParseNext(tinyxml2::XMLElement* element);
  void Clean(std::string& strDirty);
  void ConvertJSON(std::string &string);
  void ClearBuffers();
  void GetBufferParams(bool* result, const char* attribute, bool defvalue);
  void InsertToken(std::string& strOutput, int buf, const char* token);

  std::unique_ptr<CXBMCTinyXML2> m_document;

  const char* m_SearchStringEncoding;
  bool m_isNoop;

  std::string m_strFile;
  ADDON::CScraper* m_scraper;
};

