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

#include "XBMCTinyXML.h"
#include "filesystem/File.h"
#include "RegExp.h"

#define MAX_ENTITY_LENGTH 8 // size of largest entity "&#xNNNN;"
#define BUFFER_SIZE 4096

CXBMCTinyXML::CXBMCTinyXML()
: TiXmlDocument()
{
}

CXBMCTinyXML::CXBMCTinyXML(const char *documentName)
: TiXmlDocument(documentName)
{
}

CXBMCTinyXML::CXBMCTinyXML(const CStdString &documentName)
: TiXmlDocument(documentName)
{
}

bool CXBMCTinyXML::LoadFile(TiXmlEncoding encoding)
{
  return LoadFile(value, encoding);
}

bool CXBMCTinyXML::LoadFile(const char *_filename, TiXmlEncoding encoding)
{
  CStdString filename(_filename);
  return LoadFile(filename, encoding);
}

bool CXBMCTinyXML::LoadFile(const CStdString &_filename, TiXmlEncoding encoding)
{
  // There was a really terrifying little bug here. The code:
  //    value = filename
  // in the STL case, cause the assignment method of the std::string to
  // be called. What is strange, is that the std::string had the same
  // address as it's c_str() method, and so bad things happen. Looks
  // like a bug in the Microsoft STL implementation.
  // Add an extra string to avoid the crash.
  CStdString filename(_filename);
  value = filename;

  XFILE::CFileStream file;
  if (!file.Open(value))
  {
    SetError(TIXML_ERROR_OPENING_FILE, NULL, NULL, TIXML_ENCODING_UNKNOWN);
    return false;
  }

  // Delete the existing data:
  Clear();
  location.Clear();

  CStdString data;
  data.reserve(8 * 1000);
  StreamIn(&file, &data);
  file.Close();

  Parse(data, NULL, encoding);

  if (Error())
    return false;
  return true;
}

bool CXBMCTinyXML::LoadFile(FILE *f, TiXmlEncoding encoding)
{
  CStdString data("");
  char buf[BUFFER_SIZE];
  memset(buf, 0, BUFFER_SIZE);
  int result;
  while ((result = fread(buf, 1, BUFFER_SIZE, f)) > 0)
    data.append(buf, result);
  return Parse(data, NULL, encoding) != NULL;
}

bool CXBMCTinyXML::SaveFile(const char *_filename) const
{
  CStdString filename(_filename);
  return SaveFile(filename);
}

bool CXBMCTinyXML::SaveFile(const CStdString &filename) const
{
  XFILE::CFile file;
  if (file.OpenForWrite(filename, true))
  {
    TiXmlPrinter printer;
    Accept(&printer);
    file.Write(printer.CStr(), printer.Size());
    return true;
  }
  return false;
}

const char *CXBMCTinyXML::Parse(const char *_data, TiXmlParsingData *prevData, TiXmlEncoding encoding)
{
  CStdString data(_data);
  return Parse(data, prevData, encoding);
}

const char *CXBMCTinyXML::Parse(CStdString &data, TiXmlParsingData *prevData, TiXmlEncoding encoding)
{
  // Preprocess string, replacing '&' with '&amp; for invalid XML entities
  size_t pos = 0;
  CRegExp re(true);
  re.RegComp("^&(amp|lt|gt|quot|apos|#x[a-fA-F0-9]{1,4}|#[0-9]{1,5});.*");
  while ((pos = data.find("&", pos)) != CStdString::npos)
  {
    CStdString tmp = data.substr(pos, pos + MAX_ENTITY_LENGTH);
    if (re.RegFind(tmp) < 0)
      data.insert(pos + 1, "amp;");
    pos++;
  }
  return TiXmlDocument::Parse(data.c_str(), prevData, encoding);
}

bool CXBMCTinyXML::Test()
{
  // scraper results with unescaped &
  CXBMCTinyXML doc;
  CStdString data("<details><url function=\"ParseTMDBRating\" "
                  "cache=\"tmdb-en-12244.json\">"
                  "http://api.themoviedb.org/3/movie/12244"
                  "?api_key=57983e31fb435df4df77afb854740ea9"
                  "&language=en&#x3f;&#x003F;&#0063;</url></details>");
  doc.Parse(data.c_str());
  TiXmlNode *root = doc.RootElement();
  if (root && root->ValueStr() == "details")
  {
    TiXmlElement *url = root->FirstChildElement("url");
    if (url && url->FirstChild())
    {
      return (url->FirstChild()->ValueStr() == "http://api.themoviedb.org/3/movie/12244?api_key=57983e31fb435df4df77afb854740ea9&language=en???");
    }
  }
  return false;
}
