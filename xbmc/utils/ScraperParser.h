#ifndef SCRAPER_PARSER_H
#define SCRAPER_PARSER_H

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <vector>
#include "StdString.h"

#define MAX_SCRAPER_BUFFERS 20

class TiXmlElement;
class TiXmlDocument;

class CScraperSettings;

class CScraperParser
{
public:
  CScraperParser();
  CScraperParser(const CScraperParser& parser);
  ~CScraperParser();
  CScraperParser& operator= (const CScraperParser& parser);

  void Clear();
  bool Load(const CStdString& strXMLFile);
  const CStdString GetSearchStringEncoding() { return m_SearchStringEncoding; }
  const CStdString Parse(const CStdString& strTag);
  bool HasFunction(const CStdString& strTag);

  CStdString m_param[MAX_SCRAPER_BUFFERS];
  static void ClearCache();

private:
  bool LoadFromXML();
  void ReplaceBuffers(CStdString& strDest);
  void ParseExpression(const CStdString& input, CStdString& dest, TiXmlElement* element, bool bAppend);
  void ParseNext(TiXmlElement* element);
  void Clean(CStdString& strDirty);
  char* RemoveWhiteSpace(const char *string);
  void ClearBuffers();
  void GetBufferParams(bool* result, const char* attribute, bool defvalue);
  void InsertToken(CStdString& strOutput, int buf, const char* token);

  ADDON::AddonPtr m_scraper;
  TiXmlDocument* m_document;
  TiXmlElement* m_pRootElement;

  const char* m_SearchStringEncoding;

  CStdString m_strFile;
};

#endif


