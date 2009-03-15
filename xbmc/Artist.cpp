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

#include "stdafx.h"
#include "Artist.h"
#include "XMLUtils.h"
#include "Settings.h"

using namespace std;

bool CArtist::Load(const TiXmlElement *artist, bool chained)
{
  if (!artist) return false;
  if (!chained)
    Reset();

  XMLUtils::GetString(artist,"name",strArtist);
  const TiXmlNode* node = artist->FirstChild("genre");
  while (node)
  {
    if (node->FirstChild())
    {
      CStdString strTemp = node->FirstChild()->Value();
      if (strGenre.IsEmpty())
        strGenre = strTemp;
      else
        strGenre += g_advancedSettings.m_musicItemSeparator+strTemp;
    }
    node = node->NextSibling("genre");
  }
  node = artist->FirstChild("style");
  while (node)
  {
    if (node->FirstChild())
    {
      CStdString strTemp = node->FirstChild()->Value();
      if (strStyles.IsEmpty())
        strStyles = strTemp;
      else
        strStyles += g_advancedSettings.m_musicItemSeparator+strTemp;
    }
    node = node->NextSibling("style");
  }
  node = artist->FirstChild("mood");
  while (node)
  {
    if (node->FirstChild())
    {
      CStdString strTemp = node->FirstChild()->Value();
      if (strMoods.IsEmpty())
        strMoods = strTemp;
      else
        strMoods += g_advancedSettings.m_musicItemSeparator+strTemp;
    }
    node = node->NextSibling("mood");
  }

  node = artist->FirstChild("yearsactive");
  while (node)
  {
    if (node->FirstChild())
    {
      CStdString strTemp = node->FirstChild()->Value();
      if (strYearsActive.IsEmpty())
        strYearsActive = strTemp;
      else
        strYearsActive += g_advancedSettings.m_musicItemSeparator+strTemp;
    }
    node = node->NextSibling("yearsactive");
  }

  XMLUtils::GetString(artist,"born",strBorn);
  XMLUtils::GetString(artist,"formed",strFormed);
  XMLUtils::GetString(artist,"instruments",strInstruments);
  XMLUtils::GetString(artist,"biography",strBiography);
  XMLUtils::GetString(artist,"died",strDied);
  XMLUtils::GetString(artist,"disbanded",strDisbanded);

  const TiXmlElement* thumbElement = artist->FirstChildElement("thumbs");
  if (!thumbElement || !thumbURL.ParseElement(thumbElement) || thumbURL.m_url.size() == 0)
  {
    if (artist->FirstChildElement("thumb") && !artist->FirstChildElement("thumb")->FirstChildElement())
    {
      if (artist->FirstChildElement("thumb")->FirstChild() && strncmp(artist->FirstChildElement("thumb")->FirstChild()->Value(),"<thumb>",7) == 0)
      {
        CStdString strValue = artist->FirstChildElement("thumb")->FirstChild()->Value();
        TiXmlDocument doc;
        doc.Parse(strValue.c_str());
        if (doc.FirstChildElement("thumbs"))
          thumbURL.ParseElement(doc.FirstChildElement("thumbs"));
        else
          thumbURL.ParseElement(doc.FirstChildElement("thumb"));
      }
      else
        thumbURL.ParseElement(artist->FirstChildElement("thumb"));
    }
  }
  node = artist->FirstChild("album");
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
    node = node->NextSibling("album");
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
  CStdStringArray array;
  StringUtils::SplitString(strGenre, g_advancedSettings.m_musicItemSeparator,array);
  for (unsigned int i=0;i<array.size();++i)
    XMLUtils::SetString(artist,       "genre", array[i]);
  array.clear();
  StringUtils::SplitString(strStyles, g_advancedSettings.m_musicItemSeparator,array);
  for (unsigned int i=0;i<array.size();++i)
    XMLUtils::SetString(artist,      "style", array[i]);
  array.clear();
  StringUtils::SplitString(strMoods, g_advancedSettings.m_musicItemSeparator,array);
  for (unsigned int i=0;i<array.size();++i)
    XMLUtils::SetString(artist,      "mood", array[i]);
  array.clear();
  StringUtils::SplitString(strYearsActive, g_advancedSettings.m_musicItemSeparator,array);
  for (unsigned int i=0;i<array.size();++i)
    XMLUtils::SetString(artist, "yearsactive", array[i]);
  XMLUtils::SetString(artist,        "born", strBorn);
  XMLUtils::SetString(artist,      "formed", strFormed);
  XMLUtils::SetString(artist, "instruments", strInstruments);
  XMLUtils::SetString(artist,   "biography", strBiography);
  XMLUtils::SetString(artist,        "died", strDied);
  XMLUtils::SetString(artist,   "disbanded", strDisbanded);
  XMLUtils::SetString(artist,      "thumbs", thumbURL.m_xml);
  XMLUtils::SetString(artist,        "path", strPath);

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

