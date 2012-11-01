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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "VideoInfoTag.h"
#include "utils/XMLUtils.h"
#include "guilib/LocalizeStrings.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/CharsetConverter.h"
#include "TextureCache.h"
#include "filesystem/File.h"

#include <sstream>

using namespace std;

void CVideoInfoTag::Reset()
{
  m_director.clear();
  m_writingCredits.clear();
  m_genre.clear();
  m_country.clear();
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
  m_strSet.clear();
  m_iSetId = -1;
  m_tags.clear();
  m_strFile.clear();
  m_strPath.clear();
  m_strIMDBNumber.clear();
  m_strMPAARating.clear();
  m_strFileNameAndPath.clear();
  m_premiered.Reset();
  m_strStatus.clear();
  m_strProductionCode.clear();
  m_firstAired.Reset();
  m_studio.clear();
  m_strAlbum.clear();
  m_artist.clear();
  m_strTrailer.clear();
  m_iTop250 = 0;
  m_iYear = 0;
  m_iSeason = -1;
  m_iEpisode = -1;
  m_strUniqueId.clear();
  m_iSpecialSortSeason = -1;
  m_iSpecialSortEpisode = -1;
  m_fRating = 0.0f;
  m_iDbId = -1;
  m_iFileId = -1;
  m_iBookmarkId = -1;
  m_iTrack = -1;
  m_fanart.m_xml.clear();
  m_strRuntime.clear();
  m_lastPlayed.Reset();
  m_showLink.clear();
  m_streamDetails.Reset();
  m_playCount = 0;
  m_fEpBookmark = 0;
  m_basePath.clear();
  m_parentPathID = -1;
  m_resumePoint.Reset();
  m_resumePoint.type = CBookmark::RESUME;
  m_iIdShow = -1;
  m_iIdSeason = -1;
  m_strShowPath.clear();
  m_dateAdded.Reset();
  m_type.clear();
}

bool CVideoInfoTag::Save(TiXmlNode *node, const CStdString &tag, bool savePathInfo, const TiXmlElement *additionalNode)
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
    XMLUtils::SetString(movie, "uniqueid", m_strUniqueId);
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
    CXBMCTinyXML doc;
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
    CXBMCTinyXML doc;
    doc.Parse(m_fanart.m_xml);
    movie->InsertEndChild(*doc.RootElement());
  }
  XMLUtils::SetString(movie, "mpaa", m_strMPAARating);
  XMLUtils::SetInt(movie, "playcount", m_playCount);
  XMLUtils::SetDate(movie, "lastplayed", m_lastPlayed);
  if (savePathInfo)
  {
    XMLUtils::SetString(movie, "file", m_strFile);
    XMLUtils::SetString(movie, "path", m_strPath);
    XMLUtils::SetString(movie, "filenameandpath", m_strFileNameAndPath);
    XMLUtils::SetString(movie, "basepath", m_basePath);
  }
  if (!m_strEpisodeGuide.IsEmpty())
  {
    CXBMCTinyXML doc;
    doc.Parse(m_strEpisodeGuide);
    if (doc.RootElement())
      movie->InsertEndChild(*doc.RootElement());
    else
      XMLUtils::SetString(movie, "episodeguide", m_strEpisodeGuide);
  }

  XMLUtils::SetString(movie, "id", m_strIMDBNumber);
  XMLUtils::SetStringArray(movie, "genre", m_genre);
  XMLUtils::SetStringArray(movie, "country", m_country);
  XMLUtils::SetString(movie, "set", m_strSet);
  XMLUtils::SetStringArray(movie, "tag", m_tags);
  XMLUtils::SetStringArray(movie, "credits", m_writingCredits);
  XMLUtils::SetStringArray(movie, "director", m_director);
  XMLUtils::SetDate(movie, "premiered", m_premiered);
  XMLUtils::SetString(movie, "status", m_strStatus);
  XMLUtils::SetString(movie, "code", m_strProductionCode);
  XMLUtils::SetDate(movie, "aired", m_firstAired);
  XMLUtils::SetStringArray(movie, "studio", m_studio);
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
  XMLUtils::SetStringArray(movie, "artist", m_artist);
  XMLUtils::SetStringArray(movie, "showlink", m_showLink);
 
  TiXmlElement resume("resume");
  XMLUtils::SetFloat(&resume, "position", (float)m_resumePoint.timeInSeconds);
  XMLUtils::SetFloat(&resume, "total", (float)m_resumePoint.totalTimeInSeconds);
  movie->InsertEndChild(resume);

  XMLUtils::SetString(movie, "dateadded", m_dateAdded.GetAsDBDateTime());

  if (additionalNode)
    movie->InsertEndChild(*additionalNode);

  return true;
}

bool CVideoInfoTag::Load(const TiXmlElement *element, bool append, bool prioritise)
{
  if (!element)
    return false;
  if (!append)
    Reset();
  ParseNative(element, prioritise);
  return true;
}

void CVideoInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_director;
    ar << m_writingCredits;
    ar << m_genre;
    ar << m_country;
    ar << m_strTagLine;
    ar << m_strPlotOutline;
    ar << m_strPlot;
    ar << m_strPictureURL.m_spoof;
    ar << m_strPictureURL.m_xml;
    ar << m_fanart.m_xml;
    ar << m_strTitle;
    ar << m_strSortTitle;
    ar << m_strVotes;
    ar << m_studio;
    ar << m_strTrailer;
    ar << (int)m_cast.size();
    for (unsigned int i=0;i<m_cast.size();++i)
    {
      ar << m_cast[i].strName;
      ar << m_cast[i].strRole;
      ar << m_cast[i].thumb;
      ar << m_cast[i].thumbUrl.m_xml;
    }

    ar << m_strSet;
    ar << m_iSetId;
    ar << m_tags;
    ar << m_strRuntime;
    ar << m_strFile;
    ar << m_strPath;
    ar << m_strIMDBNumber;
    ar << m_strMPAARating;
    ar << m_strFileNameAndPath;
    ar << m_strOriginalTitle;
    ar << m_strEpisodeGuide;
    ar << m_premiered;
    ar << m_strStatus;
    ar << m_strProductionCode;
    ar << m_firstAired;
    ar << m_strShowTitle;
    ar << m_strAlbum;
    ar << m_artist;
    ar << m_playCount;
    ar << m_lastPlayed;
    ar << m_iTop250;
    ar << m_iYear;
    ar << m_iSeason;
    ar << m_iEpisode;
    ar << m_strUniqueId;
    ar << m_fRating;
    ar << m_iDbId;
    ar << m_iFileId;
    ar << m_iSpecialSortSeason;
    ar << m_iSpecialSortEpisode;
    ar << m_iBookmarkId;
    ar << m_iTrack;
    ar << dynamic_cast<IArchivable&>(m_streamDetails);
    ar << m_showLink;
    ar << m_fEpBookmark;
    ar << m_basePath;
    ar << m_parentPathID;
    ar << m_resumePoint.timeInSeconds;
    ar << m_resumePoint.totalTimeInSeconds;
    ar << m_iIdShow;
    ar << m_strShowPath;
    ar << m_dateAdded.GetAsDBDateTime();
    ar << m_type;
    ar << m_iIdSeason;
  }
  else
  {
    ar >> m_director;
    ar >> m_writingCredits;
    ar >> m_genre;
    ar >> m_country;
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
    ar >> m_studio;
    ar >> m_strTrailer;
    int iCastSize;
    ar >> iCastSize;
    for (int i=0;i<iCastSize;++i)
    {
      SActorInfo info;
      ar >> info.strName;
      ar >> info.strRole;
      ar >> info.thumb;
      CStdString strXml;
      ar >> strXml;
      info.thumbUrl.ParseString(strXml);
      m_cast.push_back(info);
    }

    ar >> m_strSet;
    ar >> m_iSetId;
    ar >> m_tags;
    ar >> m_strRuntime;
    ar >> m_strFile;
    ar >> m_strPath;
    ar >> m_strIMDBNumber;
    ar >> m_strMPAARating;
    ar >> m_strFileNameAndPath;
    ar >> m_strOriginalTitle;
    ar >> m_strEpisodeGuide;
    ar >> m_premiered;
    ar >> m_strStatus;
    ar >> m_strProductionCode;
    ar >> m_firstAired;
    ar >> m_strShowTitle;
    ar >> m_strAlbum;
    ar >> m_artist;
    ar >> m_playCount;
    ar >> m_lastPlayed;
    ar >> m_iTop250;
    ar >> m_iYear;
    ar >> m_iSeason;
    ar >> m_iEpisode;
    ar >> m_strUniqueId;
    ar >> m_fRating;
    ar >> m_iDbId;
    ar >> m_iFileId;
    ar >> m_iSpecialSortSeason;
    ar >> m_iSpecialSortEpisode;
    ar >> m_iBookmarkId;
    ar >> m_iTrack;
    ar >> dynamic_cast<IArchivable&>(m_streamDetails);
    ar >> m_showLink;
    ar >> m_fEpBookmark;
    ar >> m_basePath;
    ar >> m_parentPathID;
    ar >> m_resumePoint.timeInSeconds;
    ar >> m_resumePoint.totalTimeInSeconds;
    ar >> m_iIdShow;
    ar >> m_strShowPath;

    CStdString dateAdded;
    ar >> dateAdded;
    m_dateAdded.SetFromDBDateTime(dateAdded);
    ar >> m_type;
    ar >> m_iIdSeason;
  }
}

void CVideoInfoTag::Serialize(CVariant& value) const
{
  value["director"] = m_director;
  value["writer"] = m_writingCredits;
  value["genre"] = m_genre;
  value["country"] = m_country;
  value["tagline"] = m_strTagLine;
  value["plotoutline"] = m_strPlotOutline;
  value["plot"] = m_strPlot;
  value["title"] = m_strTitle;
  value["votes"] = m_strVotes;
  value["studio"] = m_studio;
  value["trailer"] = m_strTrailer;
  value["cast"] = CVariant(CVariant::VariantTypeArray);
  for (unsigned int i = 0; i < m_cast.size(); ++i)
  {
    CVariant actor;
    actor["name"] = m_cast[i].strName;
    actor["role"] = m_cast[i].strRole;
    if (!m_cast[i].thumb.IsEmpty())
      actor["thumbnail"] = CTextureCache::GetWrappedImageURL(m_cast[i].thumb);
    value["cast"].push_back(actor);
  }
  value["set"] = m_strSet;
  value["setid"] = m_iSetId;
  value["tag"] = m_tags;
  value["runtime"] = m_strRuntime;
  value["file"] = m_strFile;
  value["path"] = m_strPath;
  value["imdbnumber"] = m_strIMDBNumber;
  value["mpaa"] = m_strMPAARating;
  value["filenameandpath"] = m_strFileNameAndPath;
  value["originaltitle"] = m_strOriginalTitle;
  value["sorttitle"] = m_strSortTitle;
  value["episodeguide"] = m_strEpisodeGuide;
  value["premiered"] = m_premiered.IsValid() ? m_premiered.GetAsDBDate() : StringUtils::EmptyString;
  value["status"] = m_strStatus;
  value["productioncode"] = m_strProductionCode;
  value["firstaired"] = m_firstAired.IsValid() ? m_firstAired.GetAsDBDate() : StringUtils::EmptyString;
  value["showtitle"] = m_strShowTitle;
  value["album"] = m_strAlbum;
  value["artist"] = m_artist;
  value["playcount"] = m_playCount;
  value["lastplayed"] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDateTime() : StringUtils::EmptyString;
  value["top250"] = m_iTop250;
  value["year"] = m_iYear;
  value["season"] = m_iSeason;
  value["episode"] = m_iEpisode;
  value["uniqueid"]["unknown"] = m_strUniqueId;
  value["rating"] = m_fRating;
  value["dbid"] = m_iDbId;
  value["fileid"] = m_iFileId;
  value["track"] = m_iTrack;
  value["showlink"] = m_showLink;
  m_streamDetails.Serialize(value["streamdetails"]);
  CVariant resume = CVariant(CVariant::VariantTypeObject);
  resume["position"] = (float)m_resumePoint.timeInSeconds;
  resume["total"] = (float)m_resumePoint.totalTimeInSeconds;
  value["resume"] = resume;
  value["tvshowid"] = m_iIdShow;
  value["tvshowpath"] = m_strShowPath;
  value["dateadded"] = m_dateAdded.IsValid() ? m_dateAdded.GetAsDBDateTime() : StringUtils::EmptyString;
  value["type"] = m_type;
  value["seasonid"] = m_iIdSeason;
}

void CVideoInfoTag::ToSortable(SortItem& sortable)
{
  sortable[FieldDirector] = m_director;
  sortable[FieldWriter] = m_writingCredits;
  sortable[FieldGenre] = m_genre;
  sortable[FieldCountry] = m_country;
  sortable[FieldTagline] = m_strTagLine;
  sortable[FieldPlotOutline] = m_strPlotOutline;
  sortable[FieldPlot] = m_strPlot;
  sortable[FieldTitle] = m_strTitle;
  sortable[FieldVotes] = m_strVotes;
  sortable[FieldStudio] = m_studio;
  sortable[FieldTrailer] = m_strTrailer;
  sortable[FieldSet] = m_strSet;
  sortable[FieldTime] = m_strRuntime;
  sortable[FieldFilename] = m_strFile;
  sortable[FieldMPAA] = m_strMPAARating;
  sortable[FieldPath] = m_strFileNameAndPath;
  sortable[FieldSortTitle] = m_strSortTitle;
  sortable[FieldTvShowStatus] = m_strStatus;
  sortable[FieldProductionCode] = m_strProductionCode;
  sortable[FieldAirDate] = m_firstAired.IsValid() ? m_firstAired.GetAsDBDate() : (m_premiered.IsValid() ? m_premiered.GetAsDBDate() : StringUtils::EmptyString);
  sortable[FieldTvShowTitle] = m_strShowTitle;
  sortable[FieldAlbum] = m_strAlbum;
  sortable[FieldArtist] = m_artist;
  sortable[FieldPlaycount] = m_playCount;
  sortable[FieldLastPlayed] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDateTime() : StringUtils::EmptyString;
  sortable[FieldTop250] = m_iTop250;
  sortable[FieldYear] = m_iYear;
  sortable[FieldSeason] = m_iSeason;
  sortable[FieldEpisodeNumber] = m_iEpisode;
  sortable[FieldEpisodeNumberSpecialSort] = m_iSpecialSortEpisode;
  sortable[FieldSeasonSpecialSort] = m_iSpecialSortSeason;
  sortable[FieldRating] = m_fRating;
  sortable[FieldId] = m_iDbId;
  sortable[FieldTrackNumber] = m_iTrack;
  sortable[FieldTag] = m_tags;

  if (m_streamDetails.HasItems() && m_streamDetails.GetVideoDuration() > 0)
    sortable[FieldTime] = m_streamDetails.GetVideoDuration();
  sortable[FieldVideoResolution] = m_streamDetails.GetVideoHeight();
  sortable[FieldVideoAspectRatio] = m_streamDetails.GetVideoAspect();
  sortable[FieldVideoCodec] = m_streamDetails.GetVideoCodec();
  
  sortable[FieldAudioChannels] = m_streamDetails.GetAudioChannels();
  sortable[FieldAudioCodec] = m_streamDetails.GetAudioCodec();
  sortable[FieldAudioLanguage] = m_streamDetails.GetAudioLanguage();
  
  sortable[FieldSubtitleLanguage] = m_streamDetails.GetSubtitleLanguage();

  sortable[FieldInProgress] = m_resumePoint.IsPartWay();
  sortable[FieldDateAdded] = m_dateAdded.IsValid() ? m_dateAdded.GetAsDBDateTime() : StringUtils::EmptyString;
  sortable[FieldMediaType] = DatabaseUtils::MediaTypeFromString(m_type);
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

void CVideoInfoTag::ParseNative(const TiXmlElement* movie, bool prioritise)
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
  XMLUtils::GetString(movie, "uniqueid", m_strUniqueId);
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
  XMLUtils::GetDate(movie, "lastplayed", m_lastPlayed);
  XMLUtils::GetString(movie, "file", m_strFile);
  XMLUtils::GetString(movie, "path", m_strPath);
  XMLUtils::GetString(movie, "id", m_strIMDBNumber);
  XMLUtils::GetString(movie, "filenameandpath", m_strFileNameAndPath);
  XMLUtils::GetDate(movie, "premiered", m_premiered);
  XMLUtils::GetString(movie, "status", m_strStatus);
  XMLUtils::GetString(movie, "code", m_strProductionCode);
  XMLUtils::GetDate(movie, "aired", m_firstAired);
  XMLUtils::GetString(movie, "album", m_strAlbum);
  XMLUtils::GetString(movie, "trailer", m_strTrailer);
  XMLUtils::GetString(movie, "basepath", m_basePath);

  size_t iThumbCount = m_strPictureURL.m_url.size();
  CStdString xmlAdd = m_strPictureURL.m_xml;

  const TiXmlElement* thumb = movie->FirstChildElement("thumb");
  while (thumb)
  {
    m_strPictureURL.ParseElement(thumb);
    if (prioritise)
    {
      CStdString temp;
      temp << *thumb;
      xmlAdd = temp+xmlAdd;
    }
    thumb = thumb->NextSiblingElement("thumb");
  }

  // prioritise thumbs from nfos
  if (prioritise && iThumbCount && iThumbCount != m_strPictureURL.m_url.size())
  {
    rotate(m_strPictureURL.m_url.begin(),
           m_strPictureURL.m_url.begin()+iThumbCount, 
           m_strPictureURL.m_url.end());
    m_strPictureURL.m_xml = xmlAdd;
  }

  XMLUtils::GetStringArray(movie, "genre", m_genre, prioritise, g_advancedSettings.m_videoItemSeparator);
  XMLUtils::GetStringArray(movie, "country", m_country, prioritise, g_advancedSettings.m_videoItemSeparator);
  XMLUtils::GetStringArray(movie, "credits", m_writingCredits, prioritise, g_advancedSettings.m_videoItemSeparator);
  XMLUtils::GetStringArray(movie, "director", m_director, prioritise, g_advancedSettings.m_videoItemSeparator);
  XMLUtils::GetStringArray(movie, "showlink", m_showLink, prioritise, g_advancedSettings.m_videoItemSeparator);

  // cast
  const TiXmlElement* node = movie->FirstChildElement("actor");
  if (node && node->FirstChild() && prioritise)
    m_cast.clear();
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

  XMLUtils::GetString(movie, "set", m_strSet);
  XMLUtils::GetStringArray(movie, "tag", m_tags, prioritise, g_advancedSettings.m_videoItemSeparator);
  XMLUtils::GetStringArray(movie, "studio", m_studio, prioritise, g_advancedSettings.m_videoItemSeparator);
  // artists
  node = movie->FirstChildElement("artist");
  if (node && node->FirstChild() && prioritise)
    m_artist.clear();
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
      if (clear && stricmp(clear,"true")==0)
        m_artist.clear();
      vector<string> artists = StringUtils::Split(pValue, g_advancedSettings.m_videoItemSeparator);
      m_artist.insert(m_artist.end(), artists.begin(), artists.end());
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
    // we prioritise mixed-mode nfo's with fanart set
    if (prioritise)
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

  // dateAdded
  CStdString dateAdded;
  XMLUtils::GetString(movie, "dateadded", dateAdded);
  m_dateAdded.SetFromDBDateTime(dateAdded);
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
