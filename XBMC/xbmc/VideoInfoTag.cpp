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
#include "VideoInfoTag.h"
#include "XMLUtils.h"
#include "LocalizeStrings.h"
#include "Settings.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "Picture.h"

#include <sstream>

using namespace std;

void CVideoInfoTag::Reset()
{
  m_strDirector = "";
  m_strWritingCredits = "";
  m_strGenre = "";
  m_strTagLine = "";
  m_strPlotOutline = "";
  m_strPlot = "";
  m_strPictureURL.Clear();
  m_strTitle = "";
  m_strOriginalTitle = "";
  m_strVotes = "";
  m_cast.clear();
  m_strFile = "";
  m_strPath = "";
  m_strIMDBNumber = "";
  m_strMPAARating = "";
  m_strPremiered= "";
  m_strStatus= "";
  m_strProductionCode= "";
  m_strFirstAired= "";
  m_strStudio = "";
  m_strAlbum = "";
  m_strArtist = "";
  m_strTrailer = "";
  m_iTop250 = 0;
  m_iYear = 0;
  m_iSeason = -1;
  m_iEpisode = -1;
  m_iSpecialSortSeason = -1;
  m_iSpecialSortEpisode = -1;
  m_fRating = 0.0f;
  m_iDbId = -1;
  m_iBookmarkId = -1;
  m_iTrack = -1;
  m_fanart.m_xml = "";
  m_strRuntime = "";

  m_playCount = 0;
}

bool CVideoInfoTag::Save(TiXmlNode *node, const CStdString &tag, bool savePathInfo)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement movieElement(tag.c_str());
  TiXmlNode *movie = node->InsertEndChild(movieElement);

  if (!movie) return false;

  XMLUtils::SetString(movie, "title", m_strTitle);
  if (!m_strOriginalTitle.IsEmpty())
    XMLUtils::SetString(movie, "originaltitle", m_strOriginalTitle);
  XMLUtils::SetFloat(movie, "rating", m_fRating);
  XMLUtils::SetInt(movie, "year", m_iYear);
  XMLUtils::SetInt(movie, "top250", m_iTop250);
  if (tag == "episodedetails" || tag == "tvshow")
  {
    XMLUtils::SetInt(movie, "season", m_iSeason);
    XMLUtils::SetInt(movie, "episode", m_iEpisode);
    XMLUtils::SetInt(movie, "displayseason",m_iSpecialSortSeason);
    XMLUtils::SetInt(movie, "displayepisode",m_iSpecialSortEpisode);
  }
  if (tag == "musicvideo")
  {
    XMLUtils::SetInt(movie, "track", m_iTrack);
    XMLUtils::SetString(movie, "album", m_strAlbum);
  }
  XMLUtils::SetString(movie, "votes", m_strVotes);
  XMLUtils::SetString(movie, "outline", m_strPlotOutline);
  XMLUtils::SetString(movie, "plot", m_strPlot);
  XMLUtils::SetString(movie, "tagline", m_strTagLine);
  XMLUtils::SetString(movie, "runtime", m_strRuntime);
  XMLUtils::SetString(movie, "thumb", m_strPictureURL.m_xml);
  if (m_fanart.m_xml.size())
  {
    TiXmlDocument doc;
    doc.Parse(m_fanart.m_xml);
    movie->InsertEndChild(*doc.RootElement());
  }
  XMLUtils::SetString(movie, "mpaa", m_strMPAARating);
  XMLUtils::SetInt(movie, "playcount", m_playCount);
  if (savePathInfo)
  {
    XMLUtils::SetString(movie, "file", m_strFile);
    XMLUtils::SetString(movie, "path", m_strPath);
    XMLUtils::SetString(movie, "filenameandpath", m_strFileNameAndPath);
  }
  if (!m_strEpisodeGuide.IsEmpty())
    XMLUtils::SetString(movie, "episodeguide", m_strEpisodeGuide);

  XMLUtils::SetString(movie, "id", m_strIMDBNumber);
  XMLUtils::SetString(movie, "genre", m_strGenre);
  XMLUtils::SetString(movie, "credits", m_strWritingCredits);
  XMLUtils::SetString(movie, "director", m_strDirector);
  XMLUtils::SetString(movie, "premiered", m_strPremiered);
  XMLUtils::SetString(movie, "status", m_strStatus);
  XMLUtils::SetString(movie, "code", m_strProductionCode);
  XMLUtils::SetString(movie, "aired", m_strFirstAired);
  XMLUtils::SetString(movie, "studio", m_strStudio);
  XMLUtils::SetString(movie, "trailer", m_strTrailer);

  // cast
  for (iCast it = m_cast.begin(); it != m_cast.end(); ++it)
  {
    // add a <actor> tag
    TiXmlElement cast("actor");
    TiXmlNode *node = movie->InsertEndChild(cast);
    TiXmlElement actor("name");
    TiXmlNode *actorNode = node->InsertEndChild(actor);
    TiXmlText name(it->strName);
    actorNode->InsertEndChild(name);
    TiXmlElement role("role");
    TiXmlNode *roleNode = node->InsertEndChild(role);
    TiXmlText character(it->strRole);
    roleNode->InsertEndChild(character);
    TiXmlElement thumb("thumb");
    TiXmlNode *thumbNode = node->InsertEndChild(thumb);
    TiXmlText th(it->thumbUrl.GetFirstThumb().m_url);
    thumbNode->InsertEndChild(th);
  }
  XMLUtils::SetString(movie, "artist", m_strArtist);

  return true;
}

bool CVideoInfoTag::Load(const TiXmlElement *movie, bool chained /* = false */)
{
  if (!movie) return false;

  // reset our details if we aren't chained.
  if (!chained) Reset();

  if (CStdString("Title").Equals(movie->Value())) // mymovies.xml
    ParseMyMovies(movie);
  else
    ParseNative(movie);

  return true;
}

void CVideoInfoTag::Serialize(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_strDirector;
    ar << m_strWritingCredits;
    ar << m_strGenre;
    ar << m_strTagLine;
    ar << m_strPlotOutline;
    ar << m_strPlot;
    ar << m_strPictureURL.m_spoof;
    ar << m_strPictureURL.m_xml;
    ar << m_fanart.m_xml;
    ar << m_strTitle;
    ar << m_strVotes;
    ar << m_strStudio;
    ar << m_strTrailer;
    ar << (int)m_cast.size();
    for (unsigned int i=0;i<m_cast.size();++i)
    {
      ar << m_cast[i].strName;
      ar << m_cast[i].strRole;
      ar << m_cast[i].thumbUrl.m_xml;
    }

    ar << m_strRuntime;
    ar << m_strFile;
    ar << m_strPath;
    ar << m_strIMDBNumber;
    ar << m_strMPAARating;
    ar << m_strFileNameAndPath;
    ar << m_strOriginalTitle;
    ar << m_strEpisodeGuide;
    ar << m_strPremiered;
    ar << m_strStatus;
    ar << m_strProductionCode;
    ar << m_strFirstAired;
    ar << m_strShowTitle;
    ar << m_strAlbum;
    ar << m_strArtist;
    ar << m_playCount;
    ar << m_iTop250;
    ar << m_iYear;
    ar << m_iSeason;
    ar << m_iEpisode;
    ar << m_fRating;
    ar << m_iDbId;
    ar << m_iSpecialSortSeason;
    ar << m_iSpecialSortEpisode;
    ar << m_iBookmarkId;
    ar << m_iTrack;
  }
  else
  {
    ar >> m_strDirector;
    ar >> m_strWritingCredits;
    ar >> m_strGenre;
    ar >> m_strTagLine;
    ar >> m_strPlotOutline;
    ar >> m_strPlot;
    ar >> m_strPictureURL.m_spoof;
    ar >> m_strPictureURL.m_xml;
    m_strPictureURL.Parse();
    ar >> m_fanart.m_xml;
    m_fanart.Unpack();
    ar >> m_strTitle;
    ar >> m_strVotes;
    ar >> m_strStudio;
    ar >> m_strTrailer;
    int iCastSize;
    ar >> iCastSize;
    for (int i=0;i<iCastSize;++i)
    {
      SActorInfo info;
      ar >> info.strName;
      ar >> info.strRole;
      CStdString strXml;
      ar >> strXml;
      info.thumbUrl.ParseString(strXml);
      m_cast.push_back(info);
    }
    ar >> m_strRuntime;
    ar >> m_strFile;
    ar >> m_strPath;
    ar >> m_strIMDBNumber;
    ar >> m_strMPAARating;
    ar >> m_strFileNameAndPath;
    ar >> m_strOriginalTitle;
    ar >> m_strEpisodeGuide;
    ar >> m_strPremiered;
    ar >> m_strStatus;
    ar >> m_strProductionCode;
    ar >> m_strFirstAired;
    ar >> m_strShowTitle;
    ar >> m_strAlbum;
    ar >> m_strArtist;
    ar >> m_playCount;
    ar >> m_iTop250;
    ar >> m_iYear;
    ar >> m_iSeason;
    ar >> m_iEpisode;
    ar >> m_fRating;
    ar >> m_iDbId;
    ar >> m_iSpecialSortSeason;
    ar >> m_iSpecialSortEpisode;
    ar >> m_iBookmarkId;
    ar >> m_iTrack;
  }
}

const CStdString CVideoInfoTag::GetCast(bool bIncludeRole /*= false*/) const
{
  CStdString strLabel;
  for (iCast it = m_cast.begin(); it != m_cast.end(); ++it)
  {
    CStdString character;
    if (it->strRole.IsEmpty() || !bIncludeRole)
      character.Format("%s\n", it->strName.c_str());
    else
      character.Format("%s %s %s\n", it->strName.c_str(), g_localizeStrings.Get(20347).c_str(), it->strRole.c_str());
    strLabel += character;
  }
  return strLabel.TrimRight("\n");
}

void CVideoInfoTag::ParseNative(const TiXmlElement* movie)
{
  XMLUtils::GetString(movie, "title", m_strTitle);
  XMLUtils::GetString(movie, "originaltitle", m_strOriginalTitle);
  XMLUtils::GetFloat(movie, "rating", m_fRating);
  XMLUtils::GetInt(movie, "year", m_iYear);
  XMLUtils::GetInt(movie, "top250", m_iTop250);
  XMLUtils::GetInt(movie, "season", m_iSeason);
  XMLUtils::GetInt(movie, "episode", m_iEpisode);
  XMLUtils::GetInt(movie, "track", m_iTrack);
  XMLUtils::GetInt(movie, "displayseason", m_iSpecialSortSeason);
  XMLUtils::GetInt(movie, "displayepisode", m_iSpecialSortEpisode);
  int after=0;
  XMLUtils::GetInt(movie, "displayafterseason",after);
  if (after > 0)
  {
    m_iSpecialSortSeason = after;
    m_iSpecialSortEpisode = 0x1000; // should be more than any realistic episode number
  }
  XMLUtils::GetString(movie, "votes", m_strVotes);
  XMLUtils::GetString(movie, "outline", m_strPlotOutline);
  XMLUtils::GetString(movie, "plot", m_strPlot);
  XMLUtils::GetString(movie, "tagline", m_strTagLine);
  XMLUtils::GetString(movie, "runtime", m_strRuntime);
  XMLUtils::GetString(movie, "mpaa", m_strMPAARating);
  XMLUtils::GetInt(movie, "playcount", m_playCount);
  XMLUtils::GetString(movie, "file", m_strFile);
  XMLUtils::GetString(movie, "path", m_strPath);
  XMLUtils::GetString(movie, "id", m_strIMDBNumber);
  XMLUtils::GetString(movie, "filenameandpath", m_strFileNameAndPath);
  XMLUtils::GetString(movie, "premiered", m_strPremiered);
  XMLUtils::GetString(movie, "status", m_strStatus);
  XMLUtils::GetString(movie, "code", m_strProductionCode);
  XMLUtils::GetString(movie, "aired", m_strFirstAired);
  XMLUtils::GetString(movie, "album", m_strAlbum);
  XMLUtils::GetString(movie, "trailer", m_strTrailer);

  m_strPictureURL.ParseElement(movie->FirstChildElement("thumbs"));
  if (m_strPictureURL.m_url.size() == 0)
  { // no <thumbs> tag found, look for <thumb>
    const TiXmlElement *thumb = movie->FirstChildElement("thumb");
    if (thumb)
    {
      if (thumb->FirstChild() && !thumb->FirstChildElement())
      { // must have <thumb>child</thumb>, where child contains no XML.  Check for escaped XML
        CStdString strValue = thumb->FirstChild()->Value();
        TiXmlDocument doc;
        doc.Parse(strValue.c_str());
        if (doc.RootElement())
        { // valid XML
          if (doc.FirstChildElement("thumbs"))
            m_strPictureURL.ParseElement(doc.FirstChildElement("thumbs"));
          else if (doc.FirstChildElement("thumb"))
            m_strPictureURL.ParseElement(doc.FirstChildElement("thumb"));
        }
        else  // not XML at all - assume a single child
          m_strPictureURL.ParseElement(thumb);
      }
      else
        m_strPictureURL.ParseElement(thumb);
    }
  }

  CStdString strTemp;
  const TiXmlNode *node = movie->FirstChild("genre");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_strGenre.IsEmpty())
        m_strGenre = strTemp;
      else
        m_strGenre += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    node = node->NextSibling("genre");
  }

  node = movie->FirstChild("credits");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_strWritingCredits.IsEmpty())
        m_strWritingCredits = strTemp;
      else
        m_strWritingCredits += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    node = node->NextSibling("credits");
  }

  node = movie->FirstChild("director");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_strDirector.IsEmpty())
        m_strDirector = strTemp;
      else
        m_strDirector += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    node = node->NextSibling("director");
  }
  // cast
  node = movie->FirstChild("actor");
  while (node)
  {
    const TiXmlNode *actor = node->FirstChild("name");
    if (actor && actor->FirstChild())
    {
      SActorInfo info;
      info.strName = actor->FirstChild()->Value();
      const TiXmlNode *roleNode = node->FirstChild("role");
      if (roleNode && roleNode->FirstChild())
        info.strRole = roleNode->FirstChild()->Value();
      const TiXmlElement *thumbNode = node->FirstChildElement("thumbs");
      if (thumbNode && thumbNode->FirstChild())
        info.thumbUrl.ParseElement(thumbNode);
      else
      {
        thumbNode = node->FirstChildElement("thumb");
        if (thumbNode && thumbNode->FirstChild())
          info.thumbUrl.ParseElement(thumbNode);
      }
      m_cast.push_back(info);
    }
    node = node->NextSibling("actor");
  }
  // studios
  node = movie->FirstChild("studio");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_strStudio.IsEmpty())
        m_strStudio = strTemp;
      else
        m_strStudio += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    node = node->NextSibling("studio");
  }
  // artists
  node = movie->FirstChild("artist");
  while (node)
  {
    const TiXmlNode* pNode = node->FirstChild("name");
    const char* pValue=NULL;
    if (pNode && pNode->FirstChild())
      pValue = pNode->FirstChild()->Value();
    else if (node->FirstChild())
      pValue = node->FirstChild()->Value();
    if (pValue)
    {
      if (m_strArtist.IsEmpty())
        m_strArtist += pValue;
      else
        m_strArtist += g_advancedSettings.m_videoItemSeparator + pValue;
    }
    node = node->NextSibling("artist");
  }

  const TiXmlElement *epguide = movie->FirstChildElement("episodeguide");
  if (epguide)
  {
    if (epguide->FirstChild() && strncmp(epguide->FirstChild()->Value(),"<episodeguide>",14) == 0)
      m_strEpisodeGuide = epguide->FirstChild()->Value();
    else if (epguide->FirstChild() && strlen(epguide->FirstChild()->Value()) > 0)
    {
      stringstream stream;
      stream << *epguide;
      m_strEpisodeGuide = stream.str();
    }
  }

  // fanart
  const TiXmlElement *fanart = movie->FirstChildElement("fanart");
  if (fanart)
  {
    m_fanart.m_xml << *fanart;
    m_fanart.Unpack();
  }
}

void CVideoInfoTag::ParseMyMovies(const TiXmlElement *movie)
{
  XMLUtils::GetString(movie, "LocalTitle", m_strTitle);
  XMLUtils::GetString(movie, "OriginalTitle", m_strOriginalTitle);
  XMLUtils::GetInt(movie, "ProductionYear", m_iYear);
  int runtime = 0;
  XMLUtils::GetInt(movie, "RunningTime", runtime);
  m_strRuntime.Format("%i:%02d", runtime/60, runtime%60); // convert from minutes to hh:mm
  XMLUtils::GetString(movie, "TagLine", m_strTagLine);
  XMLUtils::GetString(movie, "Description", m_strPlot);
  if (m_strTagLine.IsEmpty())
    m_strPlotOutline = m_strPlot;

  // thumb
  CStdString strTemp;
  const TiXmlNode *node = movie->FirstChild("Covers");
  while (node)
  {
    const TiXmlNode *front = node->FirstChild("Front");
    if (front)
    {
      strTemp = front->FirstChild()->Value();
      if (!strTemp.IsEmpty())
        m_strPictureURL.ParseString(strTemp);
    }
    node = node->NextSibling("Covers");
  }
  // genres
  node = movie->FirstChild("Genres");
  const TiXmlNode *genre = node->FirstChildElement("Genre");
  while (genre)
  {
    if (genre && genre->FirstChild())
    {
      strTemp = genre->FirstChild()->Value();
      if (m_strGenre.IsEmpty())
        m_strGenre = strTemp;
      else
        m_strGenre += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    genre = genre->NextSiblingElement("Genre");
  }
  // studios
  node = movie->FirstChild("Studios");
  while (node)
  {
    const TiXmlNode *studio = node->FirstChild("Studio");
    if (studio && studio->FirstChild())
    {
      strTemp = studio->FirstChild()->Value();
      if (m_strStudio.IsEmpty())
        m_strStudio = strTemp;
      else
        m_strStudio += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    node = node->NextSibling("Studios");
  }
  // persons
  int personType = -1;
  node = movie->FirstChild("Persons");
  const TiXmlElement *element = node->FirstChildElement("Person");
  while (element)
  {
    element->Attribute("Type", &personType);
    const TiXmlNode *person = element->FirstChild("Name");
    if (person && person->FirstChild())
    {
      if (personType == 1) // actor
      {
        SActorInfo info;
        info.strName = person->FirstChild()->Value();
        const TiXmlNode *roleNode = element->FirstChild("Role");
        if (roleNode && roleNode->FirstChild())
          info.strRole = roleNode->FirstChild()->Value();
        m_cast.push_back(info);
      }
      else if (personType == 2) // director
      {
        strTemp = person->FirstChild()->Value();
        if (m_strDirector.IsEmpty())
          m_strDirector = strTemp;
        else
          m_strDirector += g_advancedSettings.m_videoItemSeparator+strTemp;
      }
    }
    element = element->NextSiblingElement("Person");
  }
}

