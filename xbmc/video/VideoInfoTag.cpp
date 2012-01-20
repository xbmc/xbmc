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

#include "VideoInfoTag.h"
#include "utils/XMLUtils.h"
#include "guilib/LocalizeStrings.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "utils/CharsetConverter.h"
#include "ThumbnailCache.h"
#include "filesystem/File.h"

#include <sstream>

using namespace std;

void CVideoInfoTag::Reset()
{
  m_strDirector.clear();
  m_strWritingCredits.clear();
  m_strGenre.clear();
  m_strCountry.clear();
  m_strTagLine.clear();
  m_strPlotOutline.clear();
  m_strPlot.clear();
  m_strPictureURL.Clear();
  m_strTitle.clear();
  m_strShowTitle.clear();
  m_strOriginalTitle.clear();
  m_strSortTitle.clear();
  m_strVotes.clear();
  m_cast.clear();
  m_set.clear();
  m_setId.clear();
  m_strFile.clear();
  m_strPath.clear();
  m_strIMDBNumber.clear();
  m_strMPAARating.clear();
  m_strFileNameAndPath.clear();
  m_strPremiered.clear();
  m_strStatus.clear();
  m_strProductionCode.clear();
  m_strFirstAired.clear();
  m_strStudio.clear();
  m_strAlbum.clear();
  m_strArtist.clear();
  m_strTrailer.clear();
  m_iTop250 = 0;
  m_iYear = 0;
  m_iSeason = -1;
  m_iEpisode = -1;
  m_iSpecialSortSeason = -1;
  m_iSpecialSortEpisode = -1;
  m_fRating = 0.0f;
  m_iDbId = -1;
  m_iFileId = -1;
  m_iBookmarkId = -1;
  m_iTrack = -1;
  m_fanart.m_xml.clear();
  m_strRuntime.clear();
  m_lastPlayed.clear();
  m_strShowLink.clear();
  m_streamDetails.Reset();
  m_playCount = 0;
  m_fEpBookmark = 0;
  m_basePath.clear();
  m_parentPathID = -1;
  m_resumePoint.Reset();
  m_resumePoint.type = CBookmark::RESUME;
  m_iIdShow = -1;
  m_strShowPath.clear();
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
  if (!m_strShowTitle.IsEmpty())
    XMLUtils::SetString(movie, "showtitle", m_strShowTitle);
  if (!m_strSortTitle.IsEmpty())
    XMLUtils::SetString(movie, "sorttitle", m_strSortTitle);
  XMLUtils::SetFloat(movie, "rating", m_fRating);
  XMLUtils::SetFloat(movie, "epbookmark", m_fEpBookmark);
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
  if (!m_strPictureURL.m_xml.empty())
  {
    TiXmlDocument doc;
    doc.Parse(m_strPictureURL.m_xml);
    const TiXmlNode* thumb = doc.FirstChild("thumb");
    while (thumb)
    {
      movie->InsertEndChild(*thumb);
      thumb = thumb->NextSibling("thumb");
    }
  }
  if (m_fanart.m_xml.size())
  {
    TiXmlDocument doc;
    doc.Parse(m_fanart.m_xml);
    movie->InsertEndChild(*doc.RootElement());
  }
  XMLUtils::SetString(movie, "mpaa", m_strMPAARating);
  XMLUtils::SetInt(movie, "playcount", m_playCount);
  XMLUtils::SetString(movie, "lastplayed", m_lastPlayed);
  if (savePathInfo)
  {
    XMLUtils::SetString(movie, "file", m_strFile);
    XMLUtils::SetString(movie, "path", m_strPath);
    XMLUtils::SetString(movie, "filenameandpath", m_strFileNameAndPath);
    XMLUtils::SetString(movie, "basepath", m_basePath);
  }
  if (!m_strEpisodeGuide.IsEmpty())
  {
    TiXmlDocument doc;
    doc.Parse(m_strEpisodeGuide);
    if (doc.RootElement())
      movie->InsertEndChild(*doc.RootElement());
    else
      XMLUtils::SetString(movie, "episodeguide", m_strEpisodeGuide);
  }

  XMLUtils::SetString(movie, "id", m_strIMDBNumber);
  XMLUtils::SetAdditiveString(movie, "genre",
                          g_advancedSettings.m_videoItemSeparator, m_strGenre);
  XMLUtils::SetAdditiveString(movie, "country",
                          g_advancedSettings.m_videoItemSeparator, m_strCountry);
  for (unsigned int i = 0; i < m_set.size(); i++)
    XMLUtils::SetString(movie, "set", m_set[i]);
  XMLUtils::SetAdditiveString(movie, "credits",
                          g_advancedSettings.m_videoItemSeparator, m_strWritingCredits);
  XMLUtils::SetAdditiveString(movie, "director",
                          g_advancedSettings.m_videoItemSeparator, m_strDirector);
  XMLUtils::SetString(movie, "premiered", m_strPremiered);
  XMLUtils::SetString(movie, "status", m_strStatus);
  XMLUtils::SetString(movie, "code", m_strProductionCode);
  XMLUtils::SetString(movie, "aired", m_strFirstAired);
  XMLUtils::SetAdditiveString(movie, "studio",
                          g_advancedSettings.m_videoItemSeparator, m_strStudio);
  XMLUtils::SetString(movie, "trailer", m_strTrailer);

  if (m_streamDetails.HasItems())
  {
    // it goes fileinfo/streamdetails/[video|audio|subtitle]
    TiXmlElement fileinfo("fileinfo");
    TiXmlElement streamdetails("streamdetails");
    for (int iStream=1; iStream<=m_streamDetails.GetVideoStreamCount(); iStream++)
    {
      TiXmlElement stream("video");
      XMLUtils::SetString(&stream, "codec", m_streamDetails.GetVideoCodec(iStream));
      XMLUtils::SetFloat(&stream, "aspect", m_streamDetails.GetVideoAspect(iStream));
      XMLUtils::SetInt(&stream, "width", m_streamDetails.GetVideoWidth(iStream));
      XMLUtils::SetInt(&stream, "height", m_streamDetails.GetVideoHeight(iStream));
      XMLUtils::SetInt(&stream, "durationinseconds", m_streamDetails.GetVideoDuration(iStream));
      streamdetails.InsertEndChild(stream);
    }
    for (int iStream=1; iStream<=m_streamDetails.GetAudioStreamCount(); iStream++)
    {
      TiXmlElement stream("audio");
      XMLUtils::SetString(&stream, "codec", m_streamDetails.GetAudioCodec(iStream));
      XMLUtils::SetString(&stream, "language", m_streamDetails.GetAudioLanguage(iStream));
      XMLUtils::SetInt(&stream, "channels", m_streamDetails.GetAudioChannels(iStream));
      streamdetails.InsertEndChild(stream);
    }
    for (int iStream=1; iStream<=m_streamDetails.GetSubtitleStreamCount(); iStream++)
    {
      TiXmlElement stream("subtitle");
      XMLUtils::SetString(&stream, "language", m_streamDetails.GetSubtitleLanguage(iStream));
      streamdetails.InsertEndChild(stream);
    }
    fileinfo.InsertEndChild(streamdetails);
    movie->InsertEndChild(fileinfo);
  }  /* if has stream details */

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
  XMLUtils::SetAdditiveString(movie, "artist",
                         g_advancedSettings.m_videoItemSeparator, m_strArtist);
  XMLUtils::SetAdditiveString(movie, "showlink",
                         g_advancedSettings.m_videoItemSeparator, m_strShowLink);
 
  TiXmlElement resume("resume");
  XMLUtils::SetFloat(&resume, "position", (float)m_resumePoint.timeInSeconds);
  XMLUtils::SetFloat(&resume, "total", (float)m_resumePoint.totalTimeInSeconds);
  movie->InsertEndChild(resume);

  return true;
}

bool CVideoInfoTag::Load(const TiXmlElement *movie, bool chained /* = false */,
                         bool prefix /* = false */)
{
  if (!movie) return false;

  // reset our details if we aren't chained.
  if (!chained) Reset();

  ParseNative(movie,prefix);

  return true;
}

void CVideoInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_strDirector;
    ar << m_strWritingCredits;
    ar << m_strGenre;
    ar << m_strCountry;
    ar << m_strTagLine;
    ar << m_strPlotOutline;
    ar << m_strPlot;
    ar << m_strPictureURL.m_spoof;
    ar << m_strPictureURL.m_xml;
    ar << m_fanart.m_xml;
    ar << m_strTitle;
    ar << m_strSortTitle;
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

    ar << (int)m_set.size();
    for (unsigned int i=0;i<m_set.size();++i)
      ar << (CStdString)m_set[i];

    ar << (int)m_setId.size();
    for (unsigned int i=0;i<m_setId.size();++i)
      ar << m_setId[i];

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
    ar << m_lastPlayed;
    ar << m_iTop250;
    ar << m_iYear;
    ar << m_iSeason;
    ar << m_iEpisode;
    ar << m_fRating;
    ar << m_iDbId;
    ar << m_iFileId;
    ar << m_iSpecialSortSeason;
    ar << m_iSpecialSortEpisode;
    ar << m_iBookmarkId;
    ar << m_iTrack;
    ar << dynamic_cast<IArchivable&>(m_streamDetails);
    ar << m_strShowLink;
    ar << m_fEpBookmark;
    ar << m_basePath;
    ar << m_parentPathID;
    ar << m_resumePoint.timeInSeconds;
    ar << m_resumePoint.totalTimeInSeconds;
    ar << m_iIdShow;
    ar << m_strShowPath;
  }
  else
  {
    ar >> m_strDirector;
    ar >> m_strWritingCredits;
    ar >> m_strGenre;
    ar >> m_strCountry;
    ar >> m_strTagLine;
    ar >> m_strPlotOutline;
    ar >> m_strPlot;
    ar >> m_strPictureURL.m_spoof;
    ar >> m_strPictureURL.m_xml;
    m_strPictureURL.Parse();
    ar >> m_fanart.m_xml;
    m_fanart.Unpack();
    ar >> m_strTitle;
    ar >> m_strSortTitle;
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

    m_set.clear();
    int iSetSize;
    ar >> iSetSize;
    for (int i=0;i<iSetSize;++i)
    {
      CStdString strSet;
      ar >> strSet;
      m_set.push_back(strSet);
    }

    m_setId.clear();
    ar >> iSetSize;
    for (int i=0;i<iSetSize;++i)
    {
      int iSetId;
      ar >> iSetId;
      m_setId.push_back(iSetId);
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
    ar >> m_lastPlayed;
    ar >> m_iTop250;
    ar >> m_iYear;
    ar >> m_iSeason;
    ar >> m_iEpisode;
    ar >> m_fRating;
    ar >> m_iDbId;
    ar >> m_iFileId;
    ar >> m_iSpecialSortSeason;
    ar >> m_iSpecialSortEpisode;
    ar >> m_iBookmarkId;
    ar >> m_iTrack;
    ar >> dynamic_cast<IArchivable&>(m_streamDetails);
    ar >> m_strShowLink;
    ar >> m_fEpBookmark;
    ar >> m_basePath;
    ar >> m_parentPathID;
    ar >> m_resumePoint.timeInSeconds;
    ar >> m_resumePoint.totalTimeInSeconds;
    ar >> m_iIdShow;
    ar >> m_strShowPath;
  }
}

void CVideoInfoTag::Serialize(CVariant& value)
{
  value["director"] = m_strDirector;
  value["writer"] = m_strWritingCredits;
  value["genre"] = m_strGenre;
  value["country"] = m_strCountry;
  value["tagline"] = m_strTagLine;
  value["plotoutline"] = m_strPlotOutline;
  value["plot"] = m_strPlot;
  value["title"] = m_strTitle;
  value["votes"] = m_strVotes;
  value["studio"] = m_strStudio;
  value["trailer"] = m_strTrailer;
  value["cast"] = CVariant(CVariant::VariantTypeArray);
  for (unsigned int i = 0; i < m_cast.size(); ++i)
  {
    CVariant actor;
    actor["name"] = m_cast[i].strName;
    actor["role"] = m_cast[i].strRole;
    CStdString thumb = CThumbnailCache::GetActorThumb(m_cast[i].strName);
    if (XFILE::CFile::Exists(thumb))
      actor["thumbnail"] = thumb;
    value["cast"].push_back(actor);
  }
  value["set"] = CVariant(CVariant::VariantTypeArray);
  for (unsigned int i = 0; i < m_set.size(); i++)
    value["set"].push_back(m_set[i]);
  value["setid"] = CVariant(CVariant::VariantTypeArray);
  for (unsigned int i = 0; i < m_setId.size(); i++)
    value["setid"].push_back(m_setId[i]);
  value["runtime"] = m_strRuntime;
  value["file"] = m_strFile;
  value["path"] = m_strPath;
  value["imdbnumber"] = m_strIMDBNumber;
  value["mpaa"] = m_strMPAARating;
  value["filenameandpath"] = m_strFileNameAndPath;
  value["originaltitle"] = m_strOriginalTitle;
  value["sorttitle"] = m_strSortTitle;
  value["episodeguide"] = m_strEpisodeGuide;
  value["premiered"] = m_strPremiered;
  value["status"] = m_strStatus;
  value["productioncode"] = m_strProductionCode;
  value["firstaired"] = m_strFirstAired;
  value["showtitle"] = m_strShowTitle;
  value["album"] = m_strAlbum;
  value["artist"] = m_strArtist;
  value["playcount"] = m_playCount;
  value["lastplayed"] = m_lastPlayed;
  value["top250"] = m_iTop250;
  value["year"] = m_iYear;
  value["season"] = m_iSeason;
  value["episode"] = m_iEpisode;
  value["rating"] = m_fRating;
  value["dbid"] = m_iDbId;
  value["fileid"] = m_iFileId;
  value["track"] = m_iTrack;
  value["showlink"] = m_strShowLink;
  m_streamDetails.Serialize(value["streamdetails"]);
  CVariant resume = CVariant(CVariant::VariantTypeObject);
  resume["position"] = (float)m_resumePoint.timeInSeconds;
  resume["total"] = (float)m_resumePoint.totalTimeInSeconds;
  value["resume"] = resume;
  value["tvshowid"] = m_iIdShow;
  value["tvshowpath"] = m_strShowPath;
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

void CVideoInfoTag::ParseNative(const TiXmlElement* movie, bool prefix)
{
  XMLUtils::GetString(movie, "title", m_strTitle);
  XMLUtils::GetString(movie, "originaltitle", m_strOriginalTitle);
  XMLUtils::GetString(movie, "showtitle", m_strShowTitle);
  XMLUtils::GetString(movie, "sorttitle", m_strSortTitle);
  XMLUtils::GetFloat(movie, "rating", m_fRating);
  XMLUtils::GetFloat(movie, "epbookmark", m_fEpBookmark);
  int max_value = 10;
  const TiXmlElement* rElement = movie->FirstChildElement("rating");
  if (rElement && (rElement->QueryIntAttribute("max", &max_value) == TIXML_SUCCESS) && max_value>=1)
  {
    m_fRating = m_fRating / max_value * 10; // Normalise the Movie Rating to between 1 and 10
  }
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
  XMLUtils::GetString(movie, "lastplayed", m_lastPlayed);
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
  XMLUtils::GetString(movie, "basepath", m_basePath);

  size_t iThumbCount = m_strPictureURL.m_url.size();
  CStdString xmlAdd = m_strPictureURL.m_xml;

  const TiXmlElement* thumb = movie->FirstChildElement("thumb");
  while (thumb)
  {
    m_strPictureURL.ParseElement(thumb);
    if (prefix)
    {
      CStdString temp;
      temp << *thumb;
      xmlAdd = temp+xmlAdd;
    }
    thumb = thumb->NextSiblingElement("thumb");
  }

  // prefix thumbs from nfos
  if (prefix && iThumbCount && iThumbCount != m_strPictureURL.m_url.size())
  {
    rotate(m_strPictureURL.m_url.begin(),
           m_strPictureURL.m_url.begin()+iThumbCount, 
           m_strPictureURL.m_url.end());
    m_strPictureURL.m_xml = xmlAdd;
  }

  XMLUtils::GetAdditiveString(movie,"genre",g_advancedSettings.m_videoItemSeparator,m_strGenre);
  XMLUtils::GetAdditiveString(movie,"country",g_advancedSettings.m_videoItemSeparator,m_strCountry);
  XMLUtils::GetAdditiveString(movie,"credits",g_advancedSettings.m_videoItemSeparator,m_strWritingCredits);
  XMLUtils::GetAdditiveString(movie,"director",g_advancedSettings.m_videoItemSeparator,m_strDirector);
  XMLUtils::GetAdditiveString(movie,"showlink",g_advancedSettings.m_videoItemSeparator,m_strShowLink);

  // cast
  const TiXmlElement* node = movie->FirstChildElement("actor");
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
      const TiXmlElement* thumb = node->FirstChildElement("thumb");
      while (thumb)
      {
        info.thumbUrl.ParseElement(thumb);
        thumb = thumb->NextSiblingElement("thumb");
      }
      const char* clear=node->Attribute("clear");
      if (clear && stricmp(clear,"true"))
        m_cast.clear();
      m_cast.push_back(info);
    }
    node = node->NextSiblingElement("actor");
  }

  node = movie->FirstChildElement("set");
  while (node)
  {
    if (node->FirstChild())
      m_set.push_back(node->FirstChild()->Value());

    node = node->NextSiblingElement("set");
  }

  XMLUtils::GetAdditiveString(movie,"studio",g_advancedSettings.m_videoItemSeparator,m_strStudio);
  // artists
  node = movie->FirstChildElement("artist");
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
      const char* clear=node->Attribute("clear");
      if (m_strArtist.IsEmpty() || (clear && stricmp(clear,"true")==0))
        m_strArtist += pValue;
      else
        m_strArtist += g_advancedSettings.m_videoItemSeparator + pValue;
    }
    node = node->NextSiblingElement("artist");
  }

  node = movie->FirstChildElement("fileinfo");
  if (node)
  {
    // Try to pull from fileinfo/streamdetails/[video|audio|subtitle]
    const TiXmlNode *nodeStreamDetails = node->FirstChild("streamdetails");
    if (nodeStreamDetails)
    {
      const TiXmlNode *nodeDetail = NULL;
      while ((nodeDetail = nodeStreamDetails->IterateChildren("audio", nodeDetail)))
      {
        CStreamDetailAudio *p = new CStreamDetailAudio();
        XMLUtils::GetString(nodeDetail, "codec", p->m_strCodec);
        XMLUtils::GetString(nodeDetail, "language", p->m_strLanguage);
        XMLUtils::GetInt(nodeDetail, "channels", p->m_iChannels);
        p->m_strCodec.MakeLower();
        p->m_strLanguage.MakeLower();
        m_streamDetails.AddStream(p);
      }
      nodeDetail = NULL;
      while ((nodeDetail = nodeStreamDetails->IterateChildren("video", nodeDetail)))
      {
        CStreamDetailVideo *p = new CStreamDetailVideo();
        XMLUtils::GetString(nodeDetail, "codec", p->m_strCodec);
        XMLUtils::GetFloat(nodeDetail, "aspect", p->m_fAspect);
        XMLUtils::GetInt(nodeDetail, "width", p->m_iWidth);
        XMLUtils::GetInt(nodeDetail, "height", p->m_iHeight);
        XMLUtils::GetInt(nodeDetail, "durationinseconds", p->m_iDuration);
        p->m_strCodec.MakeLower();
        m_streamDetails.AddStream(p);
      }
      nodeDetail = NULL;
      while ((nodeDetail = nodeStreamDetails->IterateChildren("subtitle", nodeDetail)))
      {
        CStreamDetailSubtitle *p = new CStreamDetailSubtitle();
        XMLUtils::GetString(nodeDetail, "language", p->m_strLanguage);
        p->m_strLanguage.MakeLower();
        m_streamDetails.AddStream(p);
      }
    }
    m_streamDetails.DetermineBestStreams();
  }  /* if fileinfo */

  const TiXmlElement *epguide = movie->FirstChildElement("episodeguide");
  if (epguide)
  {
    // DEPRECIATE ME - support for old XML-encoded <episodeguide> blocks.
    if (epguide->FirstChild() && strnicmp("<episodeguide", epguide->FirstChild()->Value(), 13) == 0)
      m_strEpisodeGuide = epguide->FirstChild()->Value();
    else
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
    // we prefix to handle mixed-mode nfo's with fanart set
    if (prefix)
    {
      CStdString temp;
      temp << *fanart;
      m_fanart.m_xml = temp+m_fanart.m_xml;
    }
    else
      m_fanart.m_xml << *fanart;
    m_fanart.Unpack();
  }

  // resumePoint
  const TiXmlNode *resume = movie->FirstChild("resume");
  if (resume)
  {
    XMLUtils::GetDouble(resume, "position", m_resumePoint.timeInSeconds);
    XMLUtils::GetDouble(resume, "total", m_resumePoint.totalTimeInSeconds);
  }
}

bool CVideoInfoTag::HasStreamDetails() const
{
  return m_streamDetails.HasItems();
}

bool CVideoInfoTag::IsEmpty() const
{
  return (m_strTitle.IsEmpty() &&
          m_strFile.IsEmpty() &&
          m_strPath.IsEmpty());
}
