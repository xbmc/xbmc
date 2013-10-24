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

#include "XBMCTinyXML.h"
#include "filesystem/File.h"
#include "utils/FileUtils.h"
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

CXBMCTinyXML::CXBMCTinyXML(const std::string& documentName)
: TiXmlDocument(documentName)
{
}

bool CXBMCTinyXML::LoadFile(TiXmlEncoding encoding)
{
  return LoadFile(value, encoding);
}

bool CXBMCTinyXML::LoadFile(const char *_filename, TiXmlEncoding encoding)
{
  return LoadFile(std::string(_filename), encoding);
}

bool CXBMCTinyXML::LoadFile(const std::string& _filename, TiXmlEncoding encoding)
{
  value = _filename.c_str();

  void * buffPtr;
  unsigned int buffSize = CFileUtils::LoadFile(value, buffPtr);
  if (buffSize == 0)
  {
    SetError(TIXML_ERROR_OPENING_FILE, NULL, NULL, TIXML_ENCODING_UNKNOWN);
    return false;
  }

  // Delete the existing data:
  Clear();
  location.Clear();

  std::string data ((char*) buffPtr, (size_t) buffSize);
  free(buffPtr);

  Parse(data, encoding);

  if (Error())
    return false;
  return true;
}

bool CXBMCTinyXML::LoadFile(FILE *f, TiXmlEncoding encoding)
{
  std::string data;
  char buf[BUFFER_SIZE];
  memset(buf, 0, BUFFER_SIZE);
  int result;
  while ((result = fread(buf, 1, BUFFER_SIZE, f)) > 0)
    data.append(buf, result);
  return Parse(data, encoding) != NULL;
}

bool CXBMCTinyXML::SaveFile(const char *_filename) const
{
  return SaveFile(std::string(_filename));
}

bool CXBMCTinyXML::SaveFile(const std::string& filename) const
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

bool CXBMCTinyXML::Parse(const char *_data, TiXmlEncoding encoding)
{
  return Parse(std::string(_data), encoding);
}

bool CXBMCTinyXML::Parse(const std::string& rawdata, TiXmlEncoding encoding)
{
  // Preprocess string, replacing '&' with '&amp; for invalid XML entities
  size_t pos = rawdata.find('&');
  if (pos == std::string::npos)
    return (TiXmlDocument::Parse(rawdata.c_str(), NULL, encoding) != NULL); // nothing to fix, process data directly

  std::string data(rawdata);
  CRegExp re(false, false, "^&(amp|lt|gt|quot|apos|#x[a-fA-F0-9]{1,4}|#[0-9]{1,5});.*");
  do
  {
    if (re.RegFind(data, pos, MAX_ENTITY_LENGTH) < 0)
      data.insert(pos + 1, "amp;");
    pos = data.find('&', pos + 1);
  } while (pos != std::string::npos);

  return (TiXmlDocument::Parse(data.c_str(), NULL, encoding) != NULL);
}

bool CXBMCTinyXML::Test()
{
  // scraper results with unescaped &
  CXBMCTinyXML doc;
  std::string data("<details><url function=\"ParseTMDBRating\" "
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
