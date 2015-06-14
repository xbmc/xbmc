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

#include "VideoInfoTag.h"
#include "utils/XMLUtils.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/Archive.h"
#include "TextureDatabase.h"

#include <sstream>
#include <algorithm>

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
  m_duration = 0;
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
  m_dateAdded.Reset();
  m_type.clear();
}

bool CVideoInfoTag::Save(TiXmlNode *node, const std::string &tag, bool savePathInfo, const TiXmlElement *additionalNode)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement movieElement(tag.c_str());
  TiXmlNode *movie = node->InsertEndChild(movieElement);

  if (!movie) return false;

  XMLUtils::SetString(movie, "title", m_strTitle);
  if (!m_strOriginalTitle.empty())
    XMLUtils::SetString(movie, "originaltitle", m_strOriginalTitle);
  if (!m_strShowTitle.empty())
    XMLUtils::SetString(movie, "showtitle", m_strShowTitle);
  if (!m_strSortTitle.empty())
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
  XMLUtils::SetInt(movie, "runtime", GetDuration() / 60);
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
  if (!m_strEpisodeGuide.empty())
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
      XMLUtils::SetString(&stream, "stereomode", m_streamDetails.GetStereoMode(iStream));
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
    XMLUtils::SetString(node, "name", it->strName);
    XMLUtils::SetString(node, "role", it->strRole);
    XMLUtils::SetInt(node, "order", it->order);
    XMLUtils::SetString(node, "thumb", it->thumbUrl.GetFirstThumb().m_url);
  }
  XMLUtils::SetStringArray(movie, "artist", m_artist);
  XMLUtils::SetStringArray(movie, "showlink", m_showLink);
 
  TiXmlElement resume("resume");
  XMLUtils::SetFloat(&resume, "position", (float)m_resumePoint.timeInSeconds);
  XMLUtils::SetFloat(&resume, "total", (float)m_resumePoint.totalTimeInSeconds);
  movie->InsertEndChild(resume);

  XMLUtils::SetDateTime(movie, "dateadded", m_dateAdded);

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
      ar << m_cast[i].order;
      ar << m_cast[i].thumb;
      ar << m_cast[i].thumbUrl.m_xml;
    }

    ar << m_strSet;
    ar << m_iSetId;
    ar << m_tags;
    ar << m_duration;
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
    ar >> m_fanart.m_xml;
    ar >> m_strTitle;
    ar >> m_strSortTitle;
    ar >> m_strVotes;
    ar >> m_studio;
    ar >> m_strTrailer;
    int iCastSize;
    ar >> iCastSize;
    m_cast.reserve(iCastSize);
    for (int i=0;i<iCastSize;++i)
    {
      SActorInfo info;
      ar >> info.strName;
      ar >> info.strRole;
      ar >> info.order;
      ar >> info.thumb;
      std::string strXml;
      ar >> strXml;
      info.thumbUrl.ParseString(strXml);
      m_cast.push_back(info);
    }

    ar >> m_strSet;
    ar >> m_iSetId;
    ar >> m_tags;
    ar >> m_duration;
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

    std::string dateAdded;
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
    actor["order"] = m_cast[i].order;
    if (!m_cast[i].thumb.empty())
      actor["thumbnail"] = CTextureUtils::GetWrappedImageURL(m_cast[i].thumb);
    value["cast"].push_back(actor);
  }
  value["set"] = m_strSet;
  value["setid"] = m_iSetId;
  value["tag"] = m_tags;
  value["runtime"] = GetDuration();
  value["file"] = m_strFile;
  value["path"] = m_strPath;
  value["imdbnumber"] = m_strIMDBNumber;
  value["mpaa"] = m_strMPAARating;
  value["filenameandpath"] = m_strFileNameAndPath;
  value["originaltitle"] = m_strOriginalTitle;
  value["sorttitle"] = m_strSortTitle;
  value["episodeguide"] = m_strEpisodeGuide;
  value["premiered"] = m_premiered.IsValid() ? m_premiered.GetAsDBDate() : StringUtils::Empty;
  value["status"] = m_strStatus;
  value["productioncode"] = m_strProductionCode;
  value["firstaired"] = m_firstAired.IsValid() ? m_firstAired.GetAsDBDate() : StringUtils::Empty;
  value["showtitle"] = m_strShowTitle;
  value["album"] = m_strAlbum;
  value["artist"] = m_artist;
  value["playcount"] = m_playCount;
  value["lastplayed"] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDateTime() : StringUtils::Empty;
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
  value["dateadded"] = m_dateAdded.IsValid() ? m_dateAdded.GetAsDBDateTime() : StringUtils::Empty;
  value["type"] = m_type;
  value["seasonid"] = m_iIdSeason;
  value["specialsortseason"] = m_iSpecialSortSeason;
  value["specialsortepisode"] = m_iSpecialSortEpisode;
}

void CVideoInfoTag::ToSortable(SortItem& sortable, Field field) const
{
  switch (field)
  {
  case FieldDirector:                 sortable[FieldDirector] = m_director; break;
  case FieldWriter:                   sortable[FieldWriter] = m_writingCredits; break;
  case FieldGenre:                    sortable[FieldGenre] = m_genre; break;
  case FieldCountry:                  sortable[FieldCountry] = m_country; break;
  case FieldTagline:                  sortable[FieldTagline] = m_strTagLine; break;
  case FieldPlotOutline:              sortable[FieldPlotOutline] = m_strPlotOutline; break;
  case FieldPlot:                     sortable[FieldPlot] = m_strPlot; break;
  case FieldTitle:
  {
    // make sure not to overwrite an existing title with an empty one
    std::string title = m_strTitle;
    if (!title.empty() || sortable.find(FieldTitle) == sortable.end())
      sortable[FieldTitle] = title;
    break;
  }
  case FieldVotes:                    sortable[FieldVotes] = m_strVotes; break;
  case FieldStudio:                   sortable[FieldStudio] = m_studio; break;
  case FieldTrailer:                  sortable[FieldTrailer] = m_strTrailer; break;
  case FieldSet:                      sortable[FieldSet] = m_strSet; break;
  case FieldTime:                     sortable[FieldTime] = GetDuration(); break;
  case FieldFilename:                 sortable[FieldFilename] = m_strFile; break;
  case FieldMPAA:                     sortable[FieldMPAA] = m_strMPAARating; break;
  case FieldPath:
  {
    // make sure not to overwrite an existing path with an empty one
    std::string path = GetPath();
    if (!path.empty() || sortable.find(FieldPath) == sortable.end())
      sortable[FieldPath] = path;
    break;
  }
  case FieldSortTitle:                sortable[FieldSortTitle] = m_strSortTitle; break;
  case FieldTvShowStatus:             sortable[FieldTvShowStatus] = m_strStatus; break;
  case FieldProductionCode:           sortable[FieldProductionCode] = m_strProductionCode; break;
  case FieldAirDate:                  sortable[FieldAirDate] = m_firstAired.IsValid() ? m_firstAired.GetAsDBDate() : (m_premiered.IsValid() ? m_premiered.GetAsDBDate() : StringUtils::Empty); break;
  case FieldTvShowTitle:              sortable[FieldTvShowTitle] = m_strShowTitle; break;
  case FieldAlbum:                    sortable[FieldAlbum] = m_strAlbum; break;
  case FieldArtist:                   sortable[FieldArtist] = m_artist; break;
  case FieldPlaycount:                sortable[FieldPlaycount] = m_playCount; break;
  case FieldLastPlayed:               sortable[FieldLastPlayed] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDateTime() : StringUtils::Empty; break;
  case FieldTop250:                   sortable[FieldTop250] = m_iTop250; break;
  case FieldYear:                     sortable[FieldYear] = m_iYear; break;
  case FieldSeason:                   sortable[FieldSeason] = m_iSeason; break;
  case FieldEpisodeNumber:            sortable[FieldEpisodeNumber] = m_iEpisode; break;
  case FieldEpisodeNumberSpecialSort: sortable[FieldEpisodeNumberSpecialSort] = m_iSpecialSortEpisode; break;
  case FieldSeasonSpecialSort:        sortable[FieldSeasonSpecialSort] = m_iSpecialSortSeason; break;
  case FieldRating:                   sortable[FieldRating] = m_fRating; break;
  case FieldId:                       sortable[FieldId] = m_iDbId; break;
  case FieldTrackNumber:              sortable[FieldTrackNumber] = m_iTrack; break;
  case FieldTag:                      sortable[FieldTag] = m_tags; break;

  case FieldVideoResolution:          sortable[FieldVideoResolution] = m_streamDetails.GetVideoHeight(); break;
  case FieldVideoAspectRatio:         sortable[FieldVideoAspectRatio] = m_streamDetails.GetVideoAspect(); break;
  case FieldVideoCodec:               sortable[FieldVideoCodec] = m_streamDetails.GetVideoCodec(); break;
  case FieldStereoMode:               sortable[FieldStereoMode] = m_streamDetails.GetStereoMode(); break;

  case FieldAudioChannels:            sortable[FieldAudioChannels] = m_streamDetails.GetAudioChannels(); break;
  case FieldAudioCodec:               sortable[FieldAudioCodec] = m_streamDetails.GetAudioCodec(); break;
  case FieldAudioLanguage:            sortable[FieldAudioLanguage] = m_streamDetails.GetAudioLanguage(); break;

  case FieldSubtitleLanguage:         sortable[FieldSubtitleLanguage] = m_streamDetails.GetSubtitleLanguage(); break;

  case FieldInProgress:               sortable[FieldInProgress] = m_resumePoint.IsPartWay(); break;
  case FieldDateAdded:                sortable[FieldDateAdded] = m_dateAdded.IsValid() ? m_dateAdded.GetAsDBDateTime() : StringUtils::Empty; break;
  case FieldMediaType:                sortable[FieldMediaType] = m_type; break;
  default: break;
  }
}

const std::string CVideoInfoTag::GetCast(bool bIncludeRole /*= false*/) const
{
  std::string strLabel;
  for (iCast it = m_cast.begin(); it != m_cast.end(); ++it)
  {
    std::string character;
    if (it->strRole.empty() || !bIncludeRole)
      character = StringUtils::Format("%s\n", it->strName.c_str());
    else
      character = StringUtils::Format("%s %s %s\n", it->strName.c_str(), g_localizeStrings.Get(20347).c_str(), it->strRole.c_str());
    strLabel += character;
  }
  return StringUtils::TrimRight(strLabel, "\n");
}

void CVideoInfoTag::ParseNative(const TiXmlElement* movie, bool prioritise)
{
  std::string value;

  if (XMLUtils::GetString(movie, "title", value))
    SetTitle(value);

  if (XMLUtils::GetString(movie, "originaltitle", value))
    SetOriginalTitle(value);

  if (XMLUtils::GetString(movie, "showtitle", value))
    SetShowTitle(value);

  if (XMLUtils::GetString(movie, "sorttitle", value))
    SetSortTitle(value);

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
  if (XMLUtils::GetString(movie, "uniqueid", value))
    SetUniqueId(value);

  XMLUtils::GetInt(movie, "displayseason", m_iSpecialSortSeason);
  XMLUtils::GetInt(movie, "displayepisode", m_iSpecialSortEpisode);
  int after=0;
  XMLUtils::GetInt(movie, "displayafterseason",after);
  if (after > 0)
  {
    m_iSpecialSortSeason = after;
    m_iSpecialSortEpisode = 0x1000; // should be more than any realistic episode number
  }
  if (XMLUtils::GetString(movie, "votes", value))
    SetVotes(value);

  if (XMLUtils::GetString(movie, "outline", value))
    SetPlotOutline(value);

  if (XMLUtils::GetString(movie, "plot", value))
    SetPlot(value);

  if (XMLUtils::GetString(movie, "tagline", value))
    SetTagLine(value);

  
  if (XMLUtils::GetString(movie, "runtime", value) && !value.empty())
    m_duration = GetDurationFromMinuteString(StringUtils::Trim(value));
  
  if (XMLUtils::GetString(movie, "mpaa", value))
    SetMPAARating(value);

  XMLUtils::GetInt(movie, "playcount", m_playCount);
  XMLUtils::GetDate(movie, "lastplayed", m_lastPlayed);
  
  if (XMLUtils::GetString(movie, "file", value))
    SetFile(value);

  if (XMLUtils::GetString(movie, "path", value))
    SetPath(value);

  if (XMLUtils::GetString(movie, "id", value))
    SetIMDBNumber(value);

  if (XMLUtils::GetString(movie, "filenameandpath", value))
    SetFileNameAndPath(value);

  XMLUtils::GetDate(movie, "premiered", m_premiered);
  
  if (XMLUtils::GetString(movie, "status", value))
    SetStatus(value);

  if (XMLUtils::GetString(movie, "code", value))
    SetProductionCode(value);

  XMLUtils::GetDate(movie, "aired", m_firstAired);
  
  if (XMLUtils::GetString(movie, "album", value))
    SetAlbum(value);

  if (XMLUtils::GetString(movie, "trailer", value))
    SetTrailer(value);

  if (XMLUtils::GetString(movie, "basepath", value))
    SetBasePath(value);

  size_t iThumbCount = m_strPictureURL.m_url.size();
  std::string xmlAdd = m_strPictureURL.m_xml;

  const TiXmlElement* thumb = movie->FirstChildElement("thumb");
  while (thumb)
  {
    m_strPictureURL.ParseElement(thumb);
    if (prioritise)
    {
      std::string temp;
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

  std::vector<std::string> genres(m_genre);
  if (XMLUtils::GetStringArray(movie, "genre", genres, prioritise, g_advancedSettings.m_videoItemSeparator))
    SetGenre(genres);

  std::vector<std::string> country(m_country);
  if (XMLUtils::GetStringArray(movie, "country", country, prioritise, g_advancedSettings.m_videoItemSeparator))
    SetCountry(country);

  std::vector<std::string> credits(m_writingCredits);
  if (XMLUtils::GetStringArray(movie, "credits", credits, prioritise, g_advancedSettings.m_videoItemSeparator))
    SetWritingCredits(credits);

  std::vector<std::string> director(m_director);
  if (XMLUtils::GetStringArray(movie, "director", director, prioritise, g_advancedSettings.m_videoItemSeparator))
    SetDirector(director);

  std::vector<std::string> showLink(m_showLink);
  if (XMLUtils::GetStringArray(movie, "showlink", showLink, prioritise, g_advancedSettings.m_videoItemSeparator))
    SetShowLink(showLink);

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
      
      if (XMLUtils::GetString(node, "role", value))
        info.strRole = StringUtils::Trim(value);
      
      XMLUtils::GetInt(node, "order", info.order);
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

  if (XMLUtils::GetString(movie, "set", value))
    SetSet(value);

  std::vector<std::string> tags(m_tags);
  if (XMLUtils::GetStringArray(movie, "tag", tags, prioritise, g_advancedSettings.m_videoItemSeparator))
    SetTags(tags);

  std::vector<std::string> studio(m_studio);
  if (XMLUtils::GetStringArray(movie, "studio", studio, prioritise, g_advancedSettings.m_videoItemSeparator))
    SetStudio(studio);

  // artists
  std::vector<std::string> artist(m_artist);
  node = movie->FirstChildElement("artist");
  if (node && node->FirstChild() && prioritise)
    artist.clear();
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
        artist.clear();
      vector<string> newArtists = StringUtils::Split(pValue, g_advancedSettings.m_videoItemSeparator);
      artist.insert(artist.end(), newArtists.begin(), newArtists.end());
    }
    node = node->NextSiblingElement("artist");
  }
  SetArtist(artist);

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
        if (XMLUtils::GetString(nodeDetail, "codec", value))
          p->m_strCodec = StringUtils::Trim(value);

        if (XMLUtils::GetString(nodeDetail, "language", value))
          p->m_strLanguage = StringUtils::Trim(value);

        XMLUtils::GetInt(nodeDetail, "channels", p->m_iChannels);
        StringUtils::ToLower(p->m_strCodec);
        StringUtils::ToLower(p->m_strLanguage);
        m_streamDetails.AddStream(p);
      }
      nodeDetail = NULL;
      while ((nodeDetail = nodeStreamDetails->IterateChildren("video", nodeDetail)))
      {
        CStreamDetailVideo *p = new CStreamDetailVideo();
        if (XMLUtils::GetString(nodeDetail, "codec", value))
          p->m_strCodec = StringUtils::Trim(value);

        XMLUtils::GetFloat(nodeDetail, "aspect", p->m_fAspect);
        XMLUtils::GetInt(nodeDetail, "width", p->m_iWidth);
        XMLUtils::GetInt(nodeDetail, "height", p->m_iHeight);
        XMLUtils::GetInt(nodeDetail, "durationinseconds", p->m_iDuration);
        if (XMLUtils::GetString(nodeDetail, "stereomode", value))
          p->m_strStereoMode = StringUtils::Trim(value);

        StringUtils::ToLower(p->m_strCodec);
        StringUtils::ToLower(p->m_strStereoMode);
        m_streamDetails.AddStream(p);
      }
      nodeDetail = NULL;
      while ((nodeDetail = nodeStreamDetails->IterateChildren("subtitle", nodeDetail)))
      {
        CStreamDetailSubtitle *p = new CStreamDetailSubtitle();
        if (XMLUtils::GetString(nodeDetail, "language", value))
          p->m_strLanguage = StringUtils::Trim(value);
        StringUtils::ToLower(p->m_strLanguage);
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
      std::string temp;
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

  XMLUtils::GetDateTime(movie, "dateadded", m_dateAdded);
}

bool CVideoInfoTag::HasStreamDetails() const
{
  return m_streamDetails.HasItems();
}

bool CVideoInfoTag::IsEmpty() const
{
  return (m_strTitle.empty() &&
          m_strFile.empty() &&
          m_strPath.empty());
}

unsigned int CVideoInfoTag::GetDuration() const
{
  /*
   Prefer the duration from the stream if it isn't too
   small (60%) compared to the duration from the tag.
   */
  unsigned int duration = m_streamDetails.GetVideoDuration();
  if (duration > m_duration * 0.6)
    return duration;

  return m_duration;
}

unsigned int CVideoInfoTag::GetDurationFromMinuteString(const std::string &runtime)
{
  unsigned int duration = (unsigned int)str2uint64(runtime);
  if (!duration)
  { // failed for some reason, or zero
    duration = strtoul(runtime.c_str(), NULL, 10);
    CLog::Log(LOGWARNING, "%s <runtime> should be in minutes. Interpreting '%s' as %u minutes", __FUNCTION__, runtime.c_str(), duration);
  }
  return duration*60;
}

void CVideoInfoTag::SetBasePath(std::string basePath)
{
  m_basePath = Trim(std::move(basePath));
}

void CVideoInfoTag::SetDirector(std::vector<std::string> director)
{
  m_director = Trim(std::move(director));
}

void CVideoInfoTag::SetWritingCredits(std::vector<std::string> writingCredits)
{
  m_writingCredits = Trim(std::move(writingCredits));
}

void CVideoInfoTag::SetGenre(std::vector<std::string> genre)
{
  m_genre = Trim(std::move(genre));
}

void CVideoInfoTag::SetCountry(std::vector<std::string> country)
{
  m_country = Trim(std::move(country));
}

void CVideoInfoTag::SetTagLine(std::string tagLine)
{
  m_strTagLine = Trim(std::move(tagLine));
}

void CVideoInfoTag::SetPlotOutline(std::string plotOutline)
{
  m_strPlotOutline = Trim(std::move(plotOutline));
}

void CVideoInfoTag::SetTrailer(std::string trailer)
{
  m_strTrailer = Trim(std::move(trailer));
}

void CVideoInfoTag::SetPlot(std::string plot)
{
  m_strPlot = Trim(std::move(plot));
}

void CVideoInfoTag::SetTitle(std::string title)
{
  m_strTitle = Trim(std::move(title));
}

void CVideoInfoTag::SetSortTitle(std::string sortTitle)
{
  m_strSortTitle = Trim(std::move(sortTitle));
}

void CVideoInfoTag::SetPictureURL(CScraperUrl &pictureURL)
{
  m_strPictureURL = pictureURL;
}

void CVideoInfoTag::SetVotes(std::string votes)
{
  m_strVotes = Trim(std::move(votes));
}

void CVideoInfoTag::SetArtist(std::vector<std::string> artist)
{
  m_artist = Trim(std::move(artist));
}

void CVideoInfoTag::SetSet(std::string set)
{
  m_strSet = Trim(std::move(set));
}

void CVideoInfoTag::SetTags(std::vector<std::string> tags)
{
  m_tags = Trim(std::move(tags));
}

void CVideoInfoTag::SetFile(std::string file)
{
  m_strFile = Trim(std::move(file));
}

void CVideoInfoTag::SetPath(std::string path)
{
  m_strPath = Trim(std::move(path));
}

void CVideoInfoTag::SetIMDBNumber(std::string imdbNumber)
{
  m_strIMDBNumber = Trim(std::move(imdbNumber));
}

void CVideoInfoTag::SetMPAARating(std::string mpaaRating)
{
  m_strMPAARating = Trim(std::move(mpaaRating));
}

void CVideoInfoTag::SetFileNameAndPath(std::string fileNameAndPath)
{
  m_strFileNameAndPath = Trim(std::move(fileNameAndPath));
}

void CVideoInfoTag::SetOriginalTitle(std::string originalTitle)
{
  m_strOriginalTitle = Trim(std::move(originalTitle));
}

void CVideoInfoTag::SetEpisodeGuide(std::string episodeGuide)
{
  m_strEpisodeGuide = Trim(std::move(episodeGuide));
}

void CVideoInfoTag::SetStatus(std::string status)
{
  m_strStatus = Trim(std::move(status));
}

void CVideoInfoTag::SetProductionCode(std::string productionCode)
{
  m_strProductionCode = Trim(std::move(productionCode));
}

void CVideoInfoTag::SetShowTitle(std::string showTitle)
{
  m_strShowTitle = Trim(std::move(showTitle));
}

void CVideoInfoTag::SetStudio(std::vector<std::string> studio)
{
  m_studio = Trim(std::move(studio));
}

void CVideoInfoTag::SetAlbum(std::string album)
{
  m_strAlbum = Trim(std::move(album));
}

void CVideoInfoTag::SetShowLink(std::vector<std::string> showLink)
{
  m_showLink = Trim(std::move(showLink));
}

void CVideoInfoTag::SetUniqueId(std::string uniqueId)
{
  m_strUniqueId = Trim(std::move(uniqueId));
}

std::string CVideoInfoTag::Trim(std::string &&value)
{
  return StringUtils::Trim(value);
}

std::vector<std::string> CVideoInfoTag::Trim(std::vector<std::string>&& items)
{
  std::for_each(items.begin(), items.end(), [](std::string &str){
    str = StringUtils::Trim(str);
  });
  return items;
}
