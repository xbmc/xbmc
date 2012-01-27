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

#include "Artist.h"
#include "utils/XMLUtils.h"
#include "settings/AdvancedSettings.h"

using namespace std;

bool CArtist::Load(const TiXmlElement *artist, bool append, bool prioritise)
{
  if (!artist) return false;
  if (!append)
    Reset();

  XMLUtils::GetString(artist,"name",strArtist);
  XMLUtils::GetAdditiveString(artist, "genre", g_advancedSettings.m_musicItemSeparator, strGenre, prioritise);
  XMLUtils::GetAdditiveString(artist, "style", g_advancedSettings.m_musicItemSeparator, strStyles, prioritise);
  XMLUtils::GetAdditiveString(artist, "mood", g_advancedSettings.m_musicItemSeparator, strMoods, prioritise);
  XMLUtils::GetAdditiveString(artist, "yearsactive", g_advancedSettings.m_musicItemSeparator, strYearsActive, prioritise);
  XMLUtils::GetAdditiveString(artist, "instruments", g_advancedSettings.m_musicItemSeparator, strInstruments, prioritise);

  XMLUtils::GetString(artist,"born",strBorn);
  XMLUtils::GetString(artist,"formed",strFormed);
  XMLUtils::GetString(artist,"biography",strBiography);
  XMLUtils::GetString(artist,"died",strDied);
  XMLUtils::GetString(artist,"disbanded",strDisbanded);

  size_t iThumbCount = thumbURL.m_url.size();
  CStdString xmlAdd = thumbURL.m_xml;

  const TiXmlElement* thumb = artist->FirstChildElement("thumb");
  while (thumb)
  {
    thumbURL.ParseElement(thumb);
    if (prioritise)
    {
      CStdString temp;
      temp << *thumb;
      xmlAdd = temp+xmlAdd;
    }
    thumb = thumb->NextSiblingElement("thumb");
  }
  // prefix thumbs from nfos
  if (prioritise && iThumbCount && iThumbCount != thumbURL.m_url.size())
  {
    rotate(thumbURL.m_url.begin(),
           thumbURL.m_url.begin()+iThumbCount, 
           thumbURL.m_url.end());
    thumbURL.m_xml = xmlAdd;
  }
  const TiXmlElement* node = artist->FirstChildElement("album");
  while (node)
  {
    const TiXmlNode* title = node->FirstChild("title");
    if (title && title->FirstChild())
    {
      CStdString strTitle = title->FirstChild()->Value();
      CStdString strYear;
      const TiXmlNode* year = node->FirstChild("year");
      if (year && year->FirstChild())
        strYear = year->FirstChild()->Value();
      discography.push_back(make_pair(strTitle,strYear));
    }
    node = node->NextSiblingElement("album");
  }

  // fanart
  const TiXmlElement *fanart2 = artist->FirstChildElement("fanart");
  if (fanart2)
  {
    // we prefix to handle mixed-mode nfo's with fanart set
    if (prioritise)
    {
      CStdString temp;
      temp << *fanart2;
      fanart.m_xml = temp+fanart.m_xml;
    }
    else
      fanart.m_xml << *fanart2;
    fanart.Unpack();
  }

  return true;
}

bool CArtist::Save(TiXmlNode *node, const CStdString &tag, const CStdString& strPath)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement artistElement(tag.c_str());
  TiXmlNode *artist = node->InsertEndChild(artistElement);

  if (!artist) return false;

  XMLUtils::SetString(artist,       "name", strArtist);
  XMLUtils::SetAdditiveString(artist,       "genre",
                            g_advancedSettings.m_musicItemSeparator, strGenre);
  XMLUtils::SetAdditiveString(artist,      "style",
                            g_advancedSettings.m_musicItemSeparator, strStyles);
  XMLUtils::SetAdditiveString(artist,      "mood",
                            g_advancedSettings.m_musicItemSeparator, strMoods);
  XMLUtils::SetAdditiveString(artist, "yearsactive",
                            g_advancedSettings.m_musicItemSeparator, strYearsActive);
  XMLUtils::SetAdditiveString(artist, "instruments",
                            g_advancedSettings.m_musicItemSeparator, strInstruments);
  XMLUtils::SetString(artist,        "born", strBorn);
  XMLUtils::SetString(artist,      "formed", strFormed);
  XMLUtils::SetString(artist,   "biography", strBiography);
  XMLUtils::SetString(artist,        "died", strDied);
  XMLUtils::SetString(artist,   "disbanded", strDisbanded);
  if (!thumbURL.m_xml.empty())
  {
    TiXmlDocument doc;
    doc.Parse(thumbURL.m_xml);
    const TiXmlNode* thumb = doc.FirstChild("thumb");
    while (thumb)
    {
      artist->InsertEndChild(*thumb);
      thumb = thumb->NextSibling("thumb");
    }
  }
  XMLUtils::SetString(artist,        "path", strPath);
  if (fanart.m_xml.size())
  {
    TiXmlDocument doc;
    doc.Parse(fanart.m_xml);
    artist->InsertEndChild(*doc.RootElement());
  }

  // albums
  for (vector< pair<CStdString,CStdString> >::const_iterator it = discography.begin(); it != discography.end(); ++it)
  {
    // add a <album> tag
    TiXmlElement cast("album");
    TiXmlNode *node = artist->InsertEndChild(cast);
    TiXmlElement title("title");
    TiXmlNode *titleNode = node->InsertEndChild(title);
    TiXmlText name(it->first);
    titleNode->InsertEndChild(name);
    TiXmlElement year("year");
    TiXmlNode *yearNode = node->InsertEndChild(year);
    TiXmlText name2(it->second);
    yearNode->InsertEndChild(name2);
  }

  return true;
}

