#pragma once

/*
 *      Copyright (C) 2005-2011 Team XBMC
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
#if defined(__APPLE__) || defined(WIN32)
#include "tinyXML/tinyxml.h"
#else
#include <tinyxml.h>
#endif
#ifdef USE_RENAMED_TIXMLNODE_ELEMENTS
#define ELEMENT     TINYXML_ELEMENT
#define DECLARATION TINYXML_DECLARATION
#define TEXT        TINYXML_TEXT
#endif

class CXBMCTinyXML : public TiXmlDocument
{
public:
  using TiXmlDocument::LoadFile;
  using TiXmlDocument::SaveFile;
  CXBMCTinyXML();
  CXBMCTinyXML(const char *documentName);
  bool LoadFile(const char *filename, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
  bool SaveFile(const char *filename) const;
  bool LoadFile(const std::string &filename, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING)
  {
    return LoadFile(filename.c_str(), encoding);
  }
  bool SaveFile(const std::string &filename) const
  {
    return SaveFile(filename.c_str());
  }
};
