#pragma once

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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#ifdef TARGET_WINDOWS
#define TIXML_USE_STL
#if _MSC_VER < 1700
#ifdef _DEBUG
#pragma comment(lib, "tinyxmlSTLd.lib")
#else
#pragma comment(lib, "tinyxmlSTL.lib")
#endif
#else // _MSC_VER < 1700
#ifdef _DEBUG
#pragma comment(lib, "tinyxmlSTL11d.lib")
#else
#pragma comment(lib, "tinyxmlSTL11.lib")
#endif
#endif // _MSC_VER < 1700
#else
//compile fix for TinyXml < 2.6.0
#define DOCUMENT    TINYXML_DOCUMENT
#define ELEMENT     TINYXML_ELEMENT
#define COMMENT     TINYXML_COMMENT
#define UNKNOWN     TINYXML_UNKNOWN
#define TEXT        TINYXML_TEXT
#define DECLARATION TINYXML_DECLARATION
#define TYPECOUNT   TINYXML_TYPECOUNT
#endif

#include <tinyxml.h>

#undef DOCUMENT
#undef ELEMENT
#undef COMMENT
#undef UNKNOWN
#undef TEXT
#undef DECLARATION
#undef TYPECOUNT

#include "StdString.h"

class CXBMCTinyXML : public TiXmlDocument
{
public:
  CXBMCTinyXML();
  CXBMCTinyXML(const char*);
  CXBMCTinyXML(const CStdString&);
  bool LoadFile(TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
  bool LoadFile(const char*, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
  bool LoadFile(const CStdString&, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
  bool LoadFile(FILE*, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
  bool SaveFile(const char*) const;
  bool SaveFile(const CStdString&) const;
  const char *Parse(const char*, TiXmlParsingData *prevData = NULL, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
  const char *Parse(CStdString&, TiXmlParsingData *prevData = NULL, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
  static bool Test();
};
