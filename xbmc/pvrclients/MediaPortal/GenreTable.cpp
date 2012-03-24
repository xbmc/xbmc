/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "client.h"
#include "GenreTable.h"
#include "lib/tinyxml/tinyxml.h"

using namespace ADDON;
using namespace std;

bool CGenreTable::LoadGenreXML(const std::string &filename)
{
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(filename))
  {
    XBMC->Log(LOG_DEBUG, "Unable to load %s: %s at line %d", filename.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  XBMC->Log(LOG_DEBUG, "Opened %s to read genre string to type/subtype translation table", filename.c_str());

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);
  string sGenre;
  const char* sGenreType = NULL;
  const char* sGenreSubType = NULL;
  genre_t genre;

  // block: genrestrings
  pElem = hDoc.FirstChildElement("genrestrings").Element();
  // should always have a valid root but handle gracefully if it does
  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <genrestrings> element");
    return false;
  }

  //This should hold: pElem->Value() == "genrestrings"

  // save this for later
  hRoot=TiXmlHandle(pElem);

  // iterate through all genre elements
  TiXmlElement* pGenreNode = hRoot.FirstChildElement("genre").Element();
  //This should hold: pGenreNode->Value() == "genre"

  if (!pGenreNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <genre> element");
    return false;
  }

  for (; pGenreNode != NULL; pGenreNode = pGenreNode->NextSiblingElement("genre"))
  {
    const char* sGenreString = pGenreNode->GetText();

    if (sGenreString)
    {
      sGenreType = pGenreNode->Attribute("type");
      sGenreSubType = pGenreNode->Attribute("subtype");

      if ((sGenreType) && (strlen(sGenreType) > 2))
      {
        if(sscanf(sGenreType + 2, "%x", &genre.type) != 1)
          genre.type = 0;
      }
      else
      {
        genre.type = 0;
      }

      if ((sGenreSubType) && (strlen(sGenreSubType) > 2 ))
      {
        if(sscanf(sGenreSubType + 2, "%x", &genre.subtype) != 1)
          genre.subtype = 0;
      }
      else
      {
        genre.subtype = 0;
      }

      if (genre.type > 0)
      {
        XBMC->Log(LOG_DEBUG, "Genre '%s' => 0x%x, 0x%x", sGenreString, genre.type, genre.subtype);
        m_genremap.insert(std::pair<std::string, genre_t>(sGenreString, genre));
      }
    }
  }

  return true;
}

void CGenreTable::GenreToTypes(string& strGenre, int& genreType, int& genreSubType)
{
  // The xmltv plugin from the MediaPortal TV Server can return genre
  // strings in local language (depending on the external TV guide source).
  // The only way to solve this at the XMBC side is to transfer the
  // genre string to XBMC or to let this plugin (or the TVServerXBMC
  // plugin) translate it into XBMC compatible (numbered) genre types
  string m_genre = strGenre;

  if(m_genremap.size() > 0 && m_genre.length() > 0)
  {
    GenreMap::iterator it;

    std::transform(m_genre.begin(), m_genre.end(), m_genre.begin(), ::tolower);

    it = m_genremap.find(m_genre);
    if (it != m_genremap.end())
    {
      genreType = it->second.type;
      genreSubType = it->second.subtype;
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "EPG: No mapping of '%s' to genre type/subtype found.", strGenre.c_str());
      genreType     = EPG_GENRE_USE_STRING;
      genreSubType  = 0;
    }
  }
  else
  {
    genreType = 0;
    genreSubType = 0;
  }
}
